/*
 * vx8_gps.c
 *
 *  Created on: 03 Jan 2016
 *      Author: Dmitry Melnichansky 4Z7DTF
 *  Repository: https://github.com/4z7dtf/vx8_gps
 *
 *  2016-01-03 The program created.
 *  2016-03-28 Stable version which uses TX Complete interrupt. Tested with
 *             Arduino Nano running at 16MHz and with a stand-alone ATmega328P
 *             running at 2MHz.
 *  2016-04-01 USART Buffer Empty interrupt used for TX. TX routines removed
 *             completely from the main loop. Tested with Arduino Nano at 16MHz
 *             and with a stand-alone ATmega328P running at 2MHz.
 */

/*
 No fix:
 $GPGGA,125004.000,,,,,0,00,99.9,,,,,,0000*6D
 $GPRMC,125005.000,V,,,,,,,261215,,,N*4D
 $GPZDA,125004.000,26,12,2015,,*55

 Fix:
 $GPGGA,142615.000,3226.0501,N,03454.8587,E,1,04,8.0,115.5,M,18.2,M,,0000*5B
 $GPRMC,142616.000,A,3226.0485,N,03454.8693,E,6.09,48.89,261215,,,A*57
 $GPZDA,142615.000,26,12,2015,,*52

 NEO-6M vs. Yaesu VX-8
 NEO: $GPGGA,094053.00,3204.41475,N,03445.96499,E,1,09,1.12,28.7,M,17.5,M,,*69
 VX8: $GPGGA,095142.196,4957.5953,N,00811.9616,E,0,00,99.9,00234.7,M,0047.9,M,000.0,0000*42
 VX8: $GPGGA,095142.196,4957.5953 ,N,00811.9616 ,E,0,00,99.9 ,00234.7,M,0047.9,M,000.0,0000*42
 Dif: $GPGGA,094053.00_,3204.4147X,N,03445.9649X,E,1,09,_1.1X,___28.7,M,__17.5,M,___._,____*69

 NEO: $GPRMC,094054.00,A,3204.41446,N,03445.96604,E,3.876,110.45,231215,,,A*62
 VX8: $GPRMC,095142.196,V,4957.5953,N,00811.9616,E,9999.99,999.99,080810,,*2C
 VX8: $GPRMC,095142.196,V,4957.5953 ,N,00811.9616 ,E,9999.99 ,999.99,080810,,  *2C
 Dif: $GPRMC,094054.00_,A,3204.4144X,N,03445.9660X,E,___3.87X,110.45,231215,,XX*62

 NEO: $GPZDA,142615.00,26,12,2015,,*52
 VX8: $GPZDA,095143.196,08,08,2010,,*51
 Dif: $GPZDA,142615.00_,26,12,2015,,*52

 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../src/str_func.h"

#define bool uint8_t
#define true 0x01
#define false 0x00

#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

/* Led output pin deifinitions. */
#define GGA_GREEN 0B10000000 /* GGA sentence invalid */
#define GGA_RED 0B01000000 /* GGA sentence invalid */
#define RMC_GREEN 0B00100000 /* RMC sentence valid */
#define RMC_RED 0B00010000 /* RMC sentence invalid */
#define ALL_OFF 0B00000011 /* All LEDs off */

/* Different sources state that maximum sentence length is 80 characters
 * plus CR and LF. Actual Yaesu FGPS-2 GPS output shows that this standard
 * is ignored and GGA message reaches 86 symbols. That's why the buffer sizes
 * are limited to 90 characters instead of 82.
 */
#define BUFFER_SIZE 90
#define COMMA ','
#define DOLLAR '$'
#define ASTERISK '*'
#define CR 0x0D
#define LF 0x0A
#define NULL 0x00 /* No data and string termination in RX and TX buffers. */

/* TX and RX buffers structure */
struct buffer
{
	char buffer[BUFFER_SIZE];
	uint8_t pos;
};

/* Function prototypes. */
bool process_field(void);
void reset_buffer(struct buffer *);
void copy_msg_to_tx_buf(void);
void usart_init(void);

/* RX variables */
enum rx_states
{
	READY, /* Default state, ready to receive. Changes if $ is received. */
	CMD_DETECT, /* Detecting message type (RMC, GGA etc.) Changes if comma is received. */
	RX_MESSAGE, /* Receiving the message between the $ and * delimiters. */
	RX_CHECKSUM, /* Receiving the checksum. Changes if \r\n  is received. */
	START_TX, /* Sends the message to TX when tx_has_data flag is cleared. */
	RESET, /* Resets the RX to READY state. */
};
uint8_t state; /* Current system state. */

enum nmea_commands
{
	NONE, GGA, RMC, ZDA
};
uint8_t rx_command; /* NMEA command being received. */
uint8_t rx_field_num; /* Current field of NMEA command. */
uint8_t rx_field_size; /* Current field size. */

struct buffer rx_buffer; /* Buffer for the received message. */
volatile uint8_t rx_byte; /* Received byte. */
uint8_t tbp_byte; /* Byte to process. */
uint8_t calc_checksum; /* Calculated checksum of the received message. Calculated on the fly. */
uint8_t rx_checksum; /* Checksum of the received NMEA sentence. */

/* Lookup table for converting numerical values to hexadecimal digits. */
char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/* TX variables */
struct buffer tx_buffer;
volatile bool tx_has_data; /* TX has a message to send. Set to false when tx_buffer is empty. */

int main(void)
{
	/* Setup */
	DDRD = DDRD | 0B11111100;
	PORTD &= ALL_OFF;
	usart_init();
	reset_buffer(&tx_buffer);
	rx_byte = NULL;
	state = RESET;
	sei();

	/* Main loop */
	while (1)
	{
		/* RX routine */
		if(rx_byte)
		{
			tbp_byte = rx_byte;
			rx_byte = NULL;
		}

		switch (state)
		{
		case READY:
			/* READY: The system is ready to receive and remains is this state
			 * until $ character is received.
			 */
			if (tbp_byte == DOLLAR)
			{
				rx_buffer.buffer[rx_buffer.pos] = tbp_byte;
				rx_buffer.pos++;
				state = RX_MESSAGE;
			}
			tbp_byte = NULL;
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
			if (tbp_byte)
			{
				/* If received character is $ or the buffer is overflown,
				 * reset and return to READY state.
				 */
				if (tbp_byte == DOLLAR || rx_buffer.pos >= BUFFER_SIZE)
				{
					state = RESET;
					break;
				}

				/* Comma and marks end of field, asterisk marks end of message
				 * which is also end of the last field.
				 */
				if (tbp_byte == COMMA || tbp_byte == ASTERISK)
				{
					bool field_valid = process_field();
					if (!field_valid)
					{
						state = RESET;
						break;
					}
					rx_field_num++;
					rx_field_size = 0;
				}
				else
				{
					rx_field_size++;
				}

				rx_buffer.buffer[rx_buffer.pos] = tbp_byte;
				rx_buffer.pos++;

				/* If end of message, change state to RX_CHECKSUM
				 * without affecting the calculated checksum.
				 */
				if (tbp_byte == ASTERISK)
				{
					state = RX_CHECKSUM;
				}
				else
				{
					calc_checksum ^= tbp_byte;
				}

				tbp_byte = NULL;
			}
			break;
		case RX_CHECKSUM:
			/* RX_CHECKSUM: The system receives the checksum (first two bytes
			 * after *). After CR (\r) character is received, the system
			 * compares it to the calculated value. If two values match,
			 * a new checksum is calculated and added to the message.
			 * Upon receiving the LF (\n) character which marks end of sentence
			 * system state changes to START_TX.
			 */
			if (tbp_byte)
			{
				/* If received character is $ or * or the buffer is overflown,
				 * reset and return to READY state.
				 */
				if (tbp_byte == DOLLAR || tbp_byte == ASTERISK || rx_buffer.pos >= BUFFER_SIZE)
				{
					state = RESET;
					break;
				}
				/* CR (\r) is received after the last character of checksum. */
				else if (tbp_byte == CR)
				{
					/* If match, calculate a new one, else reset. */
					if (rx_checksum == calc_checksum)
					{
						uint8_t checksum = 0x00;
						for (uint8_t i = 1; i < (rx_buffer.pos - 1); i++)
						{
							checksum ^= rx_buffer.buffer[i];
						}
						rx_buffer.buffer[rx_buffer.pos] = hex_chars[(checksum & 0xF0) >> 4];
						rx_buffer.pos++;
						rx_buffer.buffer[rx_buffer.pos] = hex_chars[checksum & 0x0F];
						rx_buffer.pos++;
						rx_buffer.buffer[rx_buffer.pos] = tbp_byte;
						rx_buffer.pos++;
					}
					else
					{
						state = RESET;
					}
				}
				/* LF (\n) is the last symbol of NMEA message. */
				else if (tbp_byte == LF)
				{
					rx_buffer.buffer[rx_buffer.pos] = tbp_byte;
					rx_buffer.pos++;
					state = START_TX;
				}
				/* Characters 0-9 and A-F are converted to numbers and added to checksum.
				 * Digit symbols have values 0x30-0x39. Capital letters start from 0x41.
				 * If the received byte is a letter (val. 0x4X) we subtract 0x07
				 * to convert the value to 0x3A-0x3F. Bitwise AND with 0x0F converts
				 * the value to 0x00-0x0F.
				 * Previous value of the received checksum is rotated 4 bits left. If the first
				 * byte of the checksum was received, the value is 0x00 and isn't affected.
				 * If the second byte is received, the value 0x0X becomes 0xX0 leaving a
				 * place for the second digit.
				 */
				else
				{
					uint8_t val = tbp_byte;
					if (val & 0x40)
						val -= 0x07;
					val &= 0x0F;
					rx_checksum <<= 4;
					rx_checksum |= val;
				}

				tbp_byte = NULL;
			}
			break;
		case START_TX:
			/* START_TX: The received and reformatted message is transferred
			 * to TX buffer. If the buffer isn't empty, the system remains in
			 * this state until the previous message is sent. After sending
			 * the message to TX system moves to READY state.
			 */
			if (!tx_has_data)
			{
				reset_buffer(&tx_buffer);
				copy_msg_to_tx_buf();
				tx_has_data = true;
				UDR0 = tx_buffer.buffer[0];
				UCSR0B |= (1 << UDRIE0); /* Enable buffer empty interrupt */
				state = RESET;
				PORTD &= ALL_OFF; /* Turn all the LEDs off. */
			}
			break;

		case RESET:
			/* RESET: resets the RX to READY state.
			 */
			reset_buffer(&rx_buffer);
			calc_checksum = 0x00;
			rx_checksum = 0x00;
			rx_command = NONE;
			rx_field_num = 0;
			rx_field_size = 0;
			tbp_byte = NULL;
			state = READY;
			break;
		}
		/* End RX routine */
	}

	return (0);
}

/*
 * Function: process_field
 * -----------------------
 *   Determines message type from the 0th field and changes the length
 *   of last received field if required.
 *
 *   returns:	True if the field was valid, false if the message has to be discarded
 *   			due to current field's value.
 */
bool process_field(void)
{
	bool res = true;

	switch (rx_command)
	{
	case NONE: /* Determine command type */
		if (rx_buffer.buffer[3] == 'G' && rx_buffer.buffer[4] == 'G' && rx_buffer.buffer[5] == 'A')
		{
			rx_command = GGA;
			break;
		}
		if (rx_buffer.buffer[3] == 'R' && rx_buffer.buffer[4] == 'M' && rx_buffer.buffer[5] == 'C')
		{
			rx_command = RMC;
			break;
		}
		if (rx_buffer.buffer[3] == 'Z' && rx_buffer.buffer[4] == 'D' && rx_buffer.buffer[5] == 'A')
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
		switch (rx_field_num)
		{
		case 0x00:
			/* Added for switch() optimization purposes. */
			break;
		case 0x01:
			/* If time field is empty all the message is discarded. */
			if (rx_field_size == 0)
			{
				PORTD |= GGA_RED; /* Turn the red LED on. */
				res = false;
				break;
			}
			/* Time field is fixed to 10 characters: hhmmss.sss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 6, 3);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 10;
			break;
		case 0x02:
			if (rx_field_size == 0)
				PORTD |= GGA_RED; /* Red LED on. */
			else
				PORTD |= GGA_GREEN; /* Green LED on. */
			/* Latitude field is fixed to 9 characters: ddmm.ssss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 4, 4);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 9;
			break;
		case 0x03:
			/* Latitude N/S field is set to N if empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'N';
				rx_buffer.pos++;
			}
			break;
		case 0x04:
			/* Longitude field is fixed to 10 characters: dddmm.ssss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 5, 4);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 10;
			break;
		case 0x05:
			/* Longitude E/W field is set to E if empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'E';
				rx_buffer.pos++;
			}
			break;
		case 0x06:
			/* Field 6 doesn't require modification. */
			break;
		case 0x07:
			/* Number of satellites is integer fixed to 2 characters. */
			fix_int_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], 2);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 2;
			break;
		case 0x08:
			/* Horizontal dilution of position field is fixed to 4 characters: xx.x.
			 * In the case of NEO-U-6 it means that one character after the decimal point
			 * will be truncated. */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 2, 1);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 4;
			break;
		case 0x09:
			/* Altitude above mean sea is fixed to 7 characters: aaaaa.a */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 5, 1);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 7;
			break;
		case 0x0A:
			/* Altitude units are set to M (meters) if this field is empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'M';
				rx_buffer.pos++;
			}
			break;
		case 0x0B:
			/* Height of geoid field is fixed to 6 characters: ddd.mm */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 4, 1);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 6;
			break;
		case 0x0C:
			/* Altitude units are set to M (meters) if this field is empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'M';
				rx_buffer.pos++;
			}
			break;
		case 0x0D:
			/* Time since last DGPS update field is fixed to 5 characters: ddd.m */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 3, 1);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 5;
			break;
		case 0x0E:
			/* DGPS station ID number is integer fixed to 4 characters. */
			fix_int_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], 4);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 4;
			break;
		}
		break;
		/* End case GGA */

	case RMC:
		/* NEO-6M vs. Yaesu VX-8
		 * NEO:    $GPRMC,094054.00,A,3204.41446,N,03445.96604,E,3.876,110.45,231215,,,A*62
		 * VX8:    $GPRMC,095142.196,V,4957.5953,N,00811.9616,E,9999.99,999.99,080810,,*2C
		 * Dif:    $GPRMC,094054.00_,A,3204.4144X,N,03445.9660X,E,___3.87X,110.45,231215,,XX*62
		 * Fields:      0          1 2          3 4           5 6        7      8      9
		 */
		switch (rx_field_num)
		{
		case 0x00:
			/* Added for switch() optimization purposes. */
			break;
		case 0x01:
			/* If time field is empty all the message is discarded. */
			if (rx_field_size == 0)
			{
				PORTD |= RMC_RED; /* Turn the red LED on. */
				res = false;
				break;
			}
			/* Time field is fixed to 10 characters: hhmmss.sss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 6, 3);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 10;
			break;
		case 0x02:
			break;
		case 0x03:
			if (rx_field_size == 0)
				PORTD |= RMC_RED; /* Red LED on. */
			else
				PORTD |= RMC_GREEN; /* Green LED on. */
			/* Latitude field is fixed to 9 characters: ddmm.ssss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 4, 4);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 9;
			break;
		case 0x04:
			/* Latitude N/S field is set to N if empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'N';
				rx_buffer.pos++;
			}
			break;
		case 0x05:
			/* Longitude field is fixed to 10 characters: dddmm.ssss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 5, 4);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 10;
			break;
		case 0x06:
			/* Longitude E/W field is set to E if empty. */
			if (rx_field_size == 0)
			{
				rx_buffer.buffer[rx_buffer.pos] = 'E';
				rx_buffer.pos++;
			}
			break;
		case 0x07:
			/* Speed field is fixed to 7 characters: ssss.ss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 4, 2);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 7;
			break;
		case 0x08:
			/* Track angle field is fixed to 6 characters: ddd.mm */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 3, 2);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 6;
			break;
		}
		break;
		/* End case RMC */

	case ZDA:
		/* NEO-6M vs. Yaesu VX-8
		 * NEO:    $GPZDA,142615.00,26,12,2015,,*52
		 * VX8:    $GPZDA,095143.196,08,08,2010,,*51
		 * Dif:    $GPZDA,142615.00_,26,12,2015,,*52
		 * Fields:      0          1  2  3    4
		 */
		if (rx_field_num == 1)
		{
			/* If time field is empty all the message is discarded. */
			if (rx_field_size == 0)
			{
				res = false;
				break;
			}
			/* Time field is fixed to 10 characters: hhmmss.sss */
			fix_decimal_field_len(&rx_buffer.buffer[rx_buffer.pos - rx_field_size], rx_field_size, 6, 3);
			rx_buffer.pos -= rx_field_size;
			rx_buffer.pos += 10;
		}
		break;
	}
	return res;
}

/*
 * Function: reset_buffer
 * ------------------
 *   Resets given buffer to empty state.
 *
 *   returns:	none
 */
void reset_buffer(struct buffer *buf)
{
	for (uint8_t i = 0; i < BUFFER_SIZE; i++)
	{
		buf->buffer[i] = NULL;
	}
	buf->pos = 0;
}

/*
 * Function: copy_msg_to_tx_buf
 * ----------------------------
 *   Copies contents of RX buffer to TX buffer.
 *
 *   returns:	none
 */
void copy_msg_to_tx_buf(void)
{
	for (uint8_t i = 0; rx_buffer.buffer[i]; i++)
	{
		tx_buffer.buffer[i] = rx_buffer.buffer[i];
	}
}


/* UART routines */
/*
 * Function: usart_init
 * --------------------
 *   Initializes UART, enables TX, RX and interrupts.
 *
 *   returns:	none
 */
void usart_init(void)
{
	/* Set baud rate */
	UBRR0H = (uint8_t) (UBRR_VALUE >> 8);
	UBRR0L = (uint8_t) UBRR_VALUE;
	/* Set frame format to 8 data bits, no parity, 1 stop bit */
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
	/* Enable reception and transmission */
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
	/* Enable RX Complete interrupt */
	UCSR0B |= (1 << RXCIE0);
}


/*
 * Function: ISR(USART_RX_vect)
 * ----------------------------
 *   RX Complete interrupt service routine.
 *
 *   returns:	none
 */
ISR(USART_RX_vect)
{
	rx_byte = UDR0;
}

/*
 * Function: ISR(USART_UDRE_vect)
 * ------------------------------
 *   UART Data Register Empty service routine. Sends next byte to buffer
 *   if end of sentence (null terminator) isn't reached. Otherwise clears
 *   tx_has_data flag and disables UART Data Register Empty interrupt.
 *
 *   returns:	none
 */
ISR(USART_UDRE_vect)
{
	if (tx_buffer.buffer[tx_buffer.pos] != NULL)
	{
		UDR0 = tx_buffer.buffer[tx_buffer.pos];
		tx_buffer.pos++;
	}
	else
	{
		UCSR0B &= ~(1 << UDRIE0); /* Disable UDR0 empty interrupt */
		tx_has_data = false;
	}
}
