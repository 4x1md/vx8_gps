/*
 * str_func.h
 *
 *  Created on: 06 jan 2016
 *  Author: Dmitry Melnichansky / 4Z7DTF
 */

#include <stdint.h>

uint8_t str_len(const char[]);
uint8_t int_len(char[]);
uint8_t frac_len(char[]);

void add_zeros_left(char[], uint8_t, uint8_t);
void rm_chars_left(char[], uint8_t, uint8_t);
void add_zeros_right(char[], uint8_t, uint8_t);
void rm_chars_right(char[], uint8_t, uint8_t);

void fix_int_field_len(char[], uint8_t);
void fix_decimal_field_len(char[], uint8_t, uint8_t);
