/*
 * vx8_gps.ino
 *
 *  Created on: 16 Jan 2016
 *      Author: 4Z7DTF
 *  Repository: https://github.com/4z7dtf/vx8_gps
 *  Decription: Arduino code of GPS module for Yaesu VX-8DR/DE transceivers.
 *              Verified with Arduino Nano 16MHz/5V on 20 March 2016.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#define bool uint8_t
#define true 0x01
#define false 0x00

/* Function prototypes. */
bool process_field(void);
void reset_rx(void);
void reset_tx(void);
void copy_msg_to_tx_buf(void);
void usart0_init(void);

#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

/* Different sources state that maximum sentence length is 80 characters
 * plus CR and LF. Actual Yaesu FGPS-2 GPS output shows that this standard
 * is ignored and GGA message reaches 86 symbols. That's why the buffer sizes
 * are limited to 90 characters instead of 82.
 */
#define BUFFER_SIZE 90
#define CR 0x0D
#define LF 0x0A

/* RX variables */
enum rx_states
{
  READY = 0x00, /* Default state, ready to receive. Changes if $ is received. */
  RX_MESSAGE = 0x01, /* Receiving the message between the $ and * delimiters. */
  RX_CHECKSUM = 0x02, /* Receiving the checksum. Changes if \r\n  is received. */
  COPY_TO_TX = 0x03, /* Default state, ready to receive. Changes if $ is received. */
};
uint8_t state; /* Current system state. */

enum nmea_commands
{
  NONE, GGA, RMC, ZDA
};
uint8_t rx_command; /* NMEA command being received. */
uint8_t rx_field; /* Current field of NMEA command. */
uint8_t rx_field_size; /* Current field size. */

char rx_buffer[BUFFER_SIZE]; /* Buffer for the received message. */
uint8_t rx_buf_pos;
bool rx_byte_ready; /* Received byte ready flag. */
uint8_t rx_byte; /* Received byte. */
uint8_t calc_checksum; /* Calculated checksum of the received message. Calculated on the fly. */
uint8_t rx_checksum; /* Checksum of the received NMEA sentence. */

/* A lookup table for converting numerical values to hexadecimal digits. */
char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/* TX variables */
char tx_buffer[BUFFER_SIZE]; /* Buffer for the message to be sent. */
uint8_t tx_buf_pos;
bool tx_not_empty; /* TX has a message to send. Set to false when tx_buffer is empty. */
bool tx_rts; /* TX ready to send. Set to true when RX is ready to send next byte. */

void setup()
{
  /* PORTD connections:
   * pin 7: green led
   * pin 5: red led
   * pin 4: green led
   * pin 2: red led
   */
  DDRD = DDRD | 0B11111100;
  PORTD = PORTD & 0B00000011;

  usart0_init();
  reset_tx();
  tx_rts = true;
  reset_rx();
  state = READY;

  sei();
}

void loop()
{
    /* Next byte is sent if there is one and TX is ready to send.
     * If null terminator or end of buffer is reached, TX is reset.
     */
    if (tx_rts && tx_not_empty)
    {
      if (tx_buf_pos < BUFFER_SIZE && tx_buffer[tx_buf_pos] != 0x00)
      {
        UDR0 = tx_buffer[tx_buf_pos];
        tx_buf_pos++;
        tx_rts = false;
      }
      else
      {
        reset_tx();
      }
    }

    /* RX routine */
    switch (state)
    {
    case READY:
      /* READY: The system is ready to receive and remains is this state
       * until $ character is received.
       */
      if (rx_byte_ready)
      {
        rx_byte_ready = false;
        if (rx_byte == '$')
        {
          rx_buffer[rx_buf_pos] = rx_byte;
          rx_buf_pos++;
          state = RX_MESSAGE;
        }
      }
      break;
    case RX_MESSAGE:
      /* RX_MESSAGE: The system receives the message between $ and *
       * delimiters. NMEA sentence checksum is calculated on the fly.
       * Comma character marks end of field. Each time it is
       * received, the field is verified and changed to VX-8 specific
       * format if required. Changing the fields on the fly results
       * in getting a new VX-8 compatible message at the end of reception.
       * NMEA sentences with empty time fields are discarded. The system
       * stops receiving current sentence and returns to READY. This is
       * done to prevent sending false time to VX-8. Other empty fields
       * are filled with default values, in most cases zeros. That's why
       * VX-8 shows zeros in coordinate fields when GPS fix isn't acquired
       * or is lost.
       * When * character is received the state changes to RX_CHECKSUM.
       */
      if (rx_byte_ready)
      {
        rx_byte_ready = false;

        /* If received character is $ or the buffer is overflown,
         * reset and return to READY state.
         */
        if (rx_byte == '$' || rx_buf_pos >= BUFFER_SIZE)
        {
          reset_rx();
          break;
        }

        /* Comma and marks end of field, asterisk marks end of message
         * which is also end of the last field.
         */
        if (rx_byte == ',' || rx_byte == '*')
        {
          bool field_valid = process_field();
          if (!field_valid)
          {
            reset_rx();
            break;
          }
          rx_field++;
          rx_field_size = 0;
        }
        else
        {
          rx_field_size++;
        }

        rx_buffer[rx_buf_pos] = rx_byte;
        rx_buf_pos++;

        /* If end of message, change state to RX_CHECKSUM
         * without affecting the calculated checksum.
         */
        if (rx_byte == '*')
        {
          state = RX_CHECKSUM;
        }
        else
        {
          calc_checksum ^= rx_byte;
        }
      }
      break;
    case RX_CHECKSUM:
      /* RX_CHECKSUM: The system receives the checksum (first two bytes
       * after *). After CR (\r) character is received, the system
       * compares it to the calculated value. If two values match,
       * a new checksum is calculated and added to the message.
       * Upon receiving the LF (\n) character which marks end of sentence
       * system state changes to COPY_TO_TX.
       */
      if (rx_byte_ready)
      {
        rx_byte_ready = false;

        /* If received character is $ or * or the buffer is overflown,
         * reset and return to READY state.
         */
        if (rx_byte == '$'|| rx_byte == '*' || rx_buf_pos >= BUFFER_SIZE)
        {
          reset_rx();
          break;
        }
        /* CR (\r) is received after the last character of checksum. */
        else if (rx_byte == CR)
        {
          /* If match, calculate a new one, else reset. */
          if (rx_checksum == calc_checksum)
          {
            uint8_t checksum = 0x00;
            for (uint8_t i = 1; i < (rx_buf_pos - 1); i++)
            {
              checksum ^= rx_buffer[i];
            }
            rx_buffer[rx_buf_pos] = hex_chars[(checksum & 0xF0) >> 4];
            rx_buf_pos++;
            rx_buffer[rx_buf_pos] = hex_chars[checksum & 0x0F];
            rx_buf_pos++;
            rx_buffer[rx_buf_pos] = rx_byte;
            rx_buf_pos++;
          }
          else
          {
            reset_rx();
          }
        }
        /* LF (\n) is the last symbol of NMEA message. */
        else if (rx_byte == LF)
        {
          rx_buffer[rx_buf_pos] = rx_byte;
          rx_buf_pos++;
          state = COPY_TO_TX;
        }
        /* Characters 0-9 and A-F are converted to numbers and added to checksum.
         * Digit symbols have values 0x30-0x39. Capital letters start from 0x41.
         * If the received byte is a letter (val. 0x4X) we substract 0x07
         * to convert the value to 0x3A-0x3F. Bitwise AND with 0x0F converts
         * the value to 0x00-0x0F.
         * Previous value of the received checksum is rotated 4 bits left. If the first
         * byte of the checksum was received, the value is 0x00 and isn't affected.
         * If the second byte is received, the value 0x0X becomes 0xX0 leaving a
         * place for the second digit.
         */
        else
        {
          uint8_t val = rx_byte;
          if (val & 0x40)
            val -= 0x07;
          val &= 0x0F;
          rx_checksum <<= 4;
          rx_checksum |= val;
        }
      }
      break;
    case COPY_TO_TX:
      /* COPY_TO_TX: The received and reformatted message is transferred
       * to TX buffer. If the buffer isn't empty, the system remains in
       * this state until the previous message is sent. After sending
       * the message to TX system moves to READY state.
       */


      /* The new message is sent to TX only if it has no message to send. */
      if (!tx_not_empty)
      {
        copy_msg_to_tx_buf();
        tx_buf_pos = 0;
        tx_not_empty = true;
        reset_rx();
      }
      PORTD = PORTD &= 0B00000000; /* Turn all the leds off. */
      break;
    }
    /* End RX routine */
  }

/*
 * Function: process_field
 * -----------------------
 *   Determines message type from the 0th field and changes the length
 *   of last received field if required.
 *
 *   returns: True if the field was valid, false if the message has to be discarded
 *        due to current field's value.
 */
bool process_field(void)
{
  bool res = true;

  switch (rx_command)
  {
  case NONE: /* Determine command type */
    if (rx_buffer[3] == 'G' && rx_buffer[4] == 'G' && rx_buffer[5] == 'A')
    {
      rx_command = GGA;
      break;
    }
    if (rx_buffer[3] == 'R' && rx_buffer[4] == 'M' && rx_buffer[5] == 'C')
    {
      rx_command = RMC;
      break;
    }
    if (rx_buffer[3] == 'Z' && rx_buffer[4] == 'D' && rx_buffer[5] == 'A')
    {
      rx_command = ZDA;
      break;
    }
    res = false;
    break;

  case GGA:
    /* NEO-6M vs. Yaesu VX-8
     * NEO:    $GPGGA,094053.00,3204.41475,N,03445.96499,E,1,09,1.12,28.7,M,17.5,M,,*69
     * VX8:    $GPGGA,095142.196,4957.5953,N,00811.9616,E,0,00,99.9,00234.7,M,0047.9,M,000.0,0000*42
     * Dif:    $GPGGA,094053.00_,3204.4147X,N,03445.9649X,E,1,09,_1.1X,___28.7,M,__17.5,M,___._,____*69
     * Fields:      0          1          2 3           4 5 6  7     8       9 A      B C     D    E
     */
    if (rx_field == 0x01)
    {
      /* If time field is empty all the message is discarded. */
      if (rx_field_size == 0)
      {
        PORTD = PORTD |= 0B00100000; /* Turn the red LED on. */
        res = false;
        break;
      }
      /* Time field is fixed to 10 characters: hhmmss.sss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 6, 3);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 10;
    }
    else if (rx_field == 0x02)
    {
      if (rx_field_size == 0)
        PORTD = PORTD |= 0B10100000; /* Red LED on. */
      else
        PORTD = PORTD |= 0B10000000; /* Green LED on. */
      /* Latitude field is fixed to 9 characters: ddmm.ssss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 4, 4);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 9;
    }
    else if (rx_field == 0x03)
    {
      /* Latitude N/S field is set to N if empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'N';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x04)
    {
      /* Longitude field is fixed to 10 characters: dddmm.ssss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 5, 4);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 10;
    }
    else if (rx_field == 0x05)
    {
      /* Longitude E/W field is set to E if empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'E';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x07)
    {
      /* Number of satellites is integer fixed to 2 characters. */
      fix_int_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 2);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 2;
    }
    else if (rx_field == 0x08)
    {
      /* Horizontal dilution of position field is fixed to 4 characters: xx.x.
       * In the case of NEO-U-6 it means that one character after the decimal point
       * will be truncated. */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 2, 1);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 4;
    }
    else if (rx_field == 0x09)
    {
      /* Altitude above mean sea is fixed to 7 characters: aaaaa.a */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 5, 1);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 7;
    }
    else if (rx_field == 0x0A)
    {
      /* Altitude units are set to M (meters) if this field is empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'M';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x0B)
    {
      /* Height of geoid field is fixed to 6 characters: ddd.mm */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 4, 1);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 6;
    }
    else if (rx_field == 0x0C)
    {
      /* Altitude units are set to M (meters) if this field is empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'M';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x0D)
    {
      /* Time since last DGPS update field is fixed to 5 characters: ddd.m */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 3, 1);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 5;
    }
    else if (rx_field == 0x0E)
    {
      /* DGPS station ID number is integer fixed to 4 characters. */
      fix_int_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 4);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 4;
    }
    break;

  case RMC:
    /* NEO-6M vs. Yaesu VX-8
     * NEO:    $GPRMC,094054.00,A,3204.41446,N,03445.96604,E,3.876,110.45,231215,,,A*62
     * VX8:    $GPRMC,095142.196,V,4957.5953,N,00811.9616,E,9999.99,999.99,080810,,*2C
     * Dif:    $GPRMC,094054.00_,A,3204.4144X,N,03445.9660X,E,___3.87X,110.45,231215,,XX*62
     * Fields:      0          1 2          3 4           5 6        7      8      9
     */
    if (rx_field == 0x01)
    {
      /* If time field is empty all the message is discarded. */
      if (rx_field_size == 0)
      {
        PORTD = PORTD |= 0B00000100; /* Turn the red LED on. */
        res = false;
        break;
      }
      /* Time field is fixed to 10 characters: hhmmss.sss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 6, 3);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 10;
    }
    else if (rx_field == 0x03)
    {
      if (rx_field_size == 0)
        PORTD = PORTD |= 0B00000100; /* Red LED on. */
      else
        PORTD = PORTD |= 0B00010000; /* Green LED on. */
      /* Latitude field is fixed to 9 characters: ddmm.ssss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 4, 4);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 9;
    }
    else if (rx_field == 0x04)
    {
      /* Latitude N/S field is set to N if empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'N';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x05)
    {
      /* Longitude field is fixed to 10 characters: dddmm.ssss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 5, 4);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 10;
    }
    else if (rx_field == 0x06)
    {
      /* Longitude E/W field is set to E if empty. */
      if (rx_field_size == 0)
      {
        rx_buffer[rx_buf_pos] = 'E';
        rx_buf_pos++;
      }
    }
    else if (rx_field == 0x07)
    {
      /* Speed field is fixed to 7 characters: ssss.ss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 4, 2);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 7;
    }
    else if (rx_field == 0x08)
    {
      /* Track angle field is fixed to 6 characters: ddd.mm */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 3, 2);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 6;
    }
    break;

  case ZDA:
    /* NEO-6M vs. Yaesu VX-8
     * NEO:    $GPZDA,142615.00,26,12,2015,,*52
     * VX8:    $GPZDA,095143.196,08,08,2010,,*51
     * Dif:    $GPZDA,142615.00_,26,12,2015,,*52
     * Fields:      0          1  2  3    4
     */
    if (rx_field == 1)
    {
      /* If time field is empty all the message is discarded. */
      if (rx_field_size == 0)
      {
        res = false;
        break;
      }
      /* Time field is fixed to 10 characters: hhmmss.sss */
      fix_decimal_field_len(&rx_buffer[rx_buf_pos - rx_field_size], 6, 3);
      rx_buf_pos -= rx_field_size;
      rx_buf_pos += 10;
    }
    break;
  }
  return res;
}

/*
 * Function: reset_tx
 * ------------------
 *   Resets transmit buffer to ready-to-send state.
 *
 *   returns: none
 */
void reset_tx(void)
{
  for (uint8_t i = 0; i < BUFFER_SIZE; i++)
  {
    tx_buffer[i] = 0x00;
  }
  tx_buf_pos = 0;
  tx_not_empty = false;
}

/*
 * Function: reset_rx
 * ------------------
 *   Resets receive buffer and sets system state to READY.
 *
 *   returns: none
 */
void reset_rx(void)
{
  for (uint8_t i = 0; i < BUFFER_SIZE; i++)
  {
    rx_buffer[i] = 0x00;
  }
  rx_buf_pos = 0;
  calc_checksum = 0x00;
  rx_checksum = 0x00;
  rx_command = NONE;
  rx_field = 0;
  rx_field_size = 0;
  state = READY;
}

/*
 * Function: copy_msg_to_tx_buf
 * ----------------------------
 *   Copies contents of RX buffer to TX buffer.
 *
 *   returns: none
 */
void copy_msg_to_tx_buf(void)
{
  for (uint8_t i = 0; i < BUFFER_SIZE; i++)
  {
    tx_buffer[i] = rx_buffer[i];
  }
}

/* UART routines */
/* UART initialization */
void usart0_init(void)
{
  // Set baud rate
  //UBRR0 = UBRR_VALUE;
  UBRR0H = (uint8_t) (UBRR_VALUE >> 8);
  UBRR0L = (uint8_t) UBRR_VALUE;
  // Set frame format to 8 data bits, no parity, 1 stop bit
  UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
  //enable reception and RC complete interrupt
  UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
  //enable transmission and UDR0 empty interrupt
  UCSR0B |= (1 << TXEN0) | (1 << UDRIE0);
}

//RX Complete interrupt service routine
ISR(USART_RX_vect)
{
  rx_byte = UDR0;
  rx_byte_ready = true;
}

ISR(USART_UDRE_vect)
{
  tx_rts = true;
}


// ============================================================================
// String functions from str_func.c library
// ============================================================================

/*
 * str_func.c
 *
 *  Created on: 06 jan 2016
 *  Author: Dmitry Melnichansky / 4Z7DTF
 */

/*
 * Function: str_len
 * -----------------
 *   Calculates the number of characters in given string.
 *
 *   str: the source string
 *
 *   returns:  the number of symbols before the \0 character.
 */
uint8_t str_len(char str[])
{
  uint8_t n = 0;
  char c = str[n];
  while (c != '\0')
  {
    n++;
    c = str[n];
  }
  return n;
}

/*
 * Function: int_len
 * -----------------
 *   Calculates the number of symbols in the integer part in the given string.
 *
 *   str: the source string
 *
 *   returns: the number of symbols before the decimal point or before the
 *        null terminator if there is no decimal point in the string.
 */
uint8_t int_len(char str[])
{
  uint8_t n = 0;
  char c = str[n];
  while (c != '.' && c != '\0')
  {
    n++;
    c = str[n];
  }
  return n;
}

/*
 * Function: frac_len
 * ------------------
 *   Calculates the number of symbols in the fraction part in the given string.
 *
 *   str: the source string
 *
 *   returns: the number of symbols after the decimal point not including
 *        the null terminator, zero if no decimal point was found.
 */
uint8_t frac_len(char str[])
{
  uint8_t pf = 0; /* point found: 1 if point was found, 0 otherwise */
  uint8_t f_len = 0; /* fraction part length */
  uint8_t n = 0; /* current character in the string */
  char c = str[n];
  while (c != '\0')
  {
    if (pf)
    {
      f_len++;
    }
    else if(c == '.')
    {
      pf = 1;
    }
    n++;
    c = str[n];
  }
  return f_len;
}

/*
 * Function: add_zeros_left
 * ------------------------
 *   Adds leading zeros to the given string.
 *
 *   str: the source string
 *   src_len: source string length
 *   new_len: required string length
 *
 *   returns: none
 */
void add_zeros_left(char str[], uint8_t src_len, uint8_t new_len)
{
  uint8_t diff = new_len - src_len;
  for (int8_t i = new_len; i >= 0; i--)
  {
    if (i >= diff)
    {
      str[i] = str[i - diff];
    }
    else
    {
      str[i] = '0';
    }
  }
}

/*
 * Function: rm_zeros_left
 * -----------------------
 *   Removes leading characters from the given string.
 *
 *   str: the source string
 *   src_len: source string length
 *   new_len: required string length
 *
 *   returns: none
 */
void rm_chars_left(char str[], uint8_t src_len, uint8_t new_len)
{
  uint8_t diff = src_len - new_len;
  for (int8_t i = 0; i <= new_len; i++)
  {
    str[i] = str[i + diff];
  }
}

/*
 * Function: add_zeros_right
 * -------------------------
 *   Appends zeros to the given string.
 *
 *   str: the source string
 *   src_len: source string length
 *   new_len: required string length
 *
 *   returns: none
 */
void add_zeros_right(char str[], uint8_t src_len, uint8_t new_len)
{
  for (int8_t i = src_len; i < new_len; i++)
  {
    str[i] = '0';
  }
  str[new_len] = '\0';
}

/*
 * Function: rm_chars_left
 * -----------------------
 *   Removes characters at the end of the given string.
 *
 *   str: the source string
 *   src_len: source string length
 *   new_len: required string length
 *
 *   returns: none
 */
void rm_chars_right(char str[], uint8_t src_len, uint8_t new_len)
{
  str[new_len] = '\0';
}

/*
 * Function: fix_int_field_len
 * ---------------------------
 *   Sets size of a GPS field containing an integer. Adds leading zeros if
 *   required length is bigger than the length of the source string. Removes
 *   leading characters if required length is smaller than the length of the
 *   source string.
 *
 *   str: the source string
 *   new_len: required string length
 *
 *   returns: none
 */
void fix_int_field_len(char field[], uint8_t new_len)
{
  uint8_t src_len = str_len(field);
  if (src_len == new_len)
  {
    return;
  }
  if (new_len > src_len)
  {
    add_zeros_left(field, src_len, new_len);
  }
  else
  {
    rm_chars_left(field, src_len, new_len);
  }
}


/*
 * Function: fix_decimal_field_len
 * -------------------------------
 *   Sets size of a GPS field containing a decimal number. Adds leading zeros
 *   or removes leading characters if required. Adds zeros at the end or
 *   removes characters at the end if required.
 *
 *   str: the source string
 *   new_int_len: the new length of the integer part
 *   new_frac_len: the new length of the fractional part
 *
 *   returns: none
 */
void fix_decimal_field_len(char field[], uint8_t new_int_len, uint8_t new_frac_len)
{
  uint8_t field_len = str_len(field);

  /* If the field is empty, decimal point is added before adding zeros. */
  if (!field_len)
  {
    field[0] = '.';
    field[1] = '\0';
    field_len++;
  }

  /* Integer part: leading zeros are added or removed. */
  uint8_t i_len = int_len(field);
  if (i_len != new_int_len)
  {
    uint8_t new_len = field_len - i_len + new_int_len;
    if (new_int_len > i_len)
    {
      add_zeros_left(field, field_len, new_len);
    }
    else
    {
      rm_chars_left(field, field_len, new_len);
    }
    field_len = new_len;
  }

  /* Fraction part: zeros at the end added or removed. */
  uint8_t f_len = frac_len(field);
  if (f_len != new_frac_len)
  {
    uint8_t new_len = field_len - f_len + new_frac_len;
    if (new_frac_len > f_len)
    {
      add_zeros_right(field, field_len, new_len);
    }
    else
    {
      rm_chars_right(field, field_len, new_len);
    }
  }
}

