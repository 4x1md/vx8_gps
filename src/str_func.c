/*
 * str_func.c
 *
 *  Created on: 06 jan 2016
 *  Author: Dmitry Melnichansky / 4Z7DTF
 */

#include "../src/str_func.h"
#include <stdlib.h>

/*
 * Function: str_len
 * -----------------
 *   Calculates the number of characters in given string.
 *
 *   str: the source string
 *
 *   returns:	the number of symbols before the \0 character.
 */
uint8_t str_len(const char *str)
{
	const char *s;
	for (s = str; *s; ++s);
	return (s - str);
}

/*
 * Function: int_len
 * -----------------
 *   Calculates the number of symbols in the integer part in the given string.
 *
 *   str: the source string
 *
 *   returns:	the number of symbols before the decimal point or before the
 *   			null terminator if there is no decimal point in the string.
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
 *   returns:	the number of symbols after the decimal point not including
 *   			the null terminator, zero if no decimal point was found.
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
 *   returns:	none
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
 *   returns:	none
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
 *   returns:	none
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
 *   returns:	none
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
 *   returns:	none
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
 *   field_len: source field length
 *   new_int_len: the new length of the integer part
 *   new_frac_len: the new length of the fractional part
 *
 *   returns:	none
 */
void fix_decimal_field_len(char field[], uint8_t field_len, uint8_t new_int_len, uint8_t new_frac_len)
{
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
