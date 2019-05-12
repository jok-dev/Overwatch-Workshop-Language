#pragma once

#include "general.h"

#include <stdarg.h>

struct dynstr
{
	char* raw;

	// @Todo: pretty short string lengths for a compiler..
	u16 len;

	// @Todo: hide this?
	u16 buf_len;
};

dynstr* dynstr_create(const char* str);
dynstr* dynstr_create(const char* str, u32 len);
dynstr* dynstr_create(u32 buf_len);
dynstr* dynstr_create(dynstr* dstr);

void dynstr_destroy(dynstr* str);

dynstr* dynstr_append_char(dynstr* to, char from);
dynstr* dynstr_append_char_repeat(dynstr* to, char from, u32 count);
dynstr* dynstr_append_str(dynstr* to, const char* from);
dynstr* dynstr_append_str(dynstr* to, const char* from, u32 from_len);

dynstr* dynstr_append_int(dynstr* to, int from);

// takes a variable number of char* to append
dynstr* dynstr_append(dynstr* to, const char* format, ...);
dynstr* dynstr_append_va(dynstr* to, const char* format, va_list args);

dynstr* dynstr_append(dynstr* to, dynstr* from);
dynstr* dynstr_clear(dynstr* dstr);

// move the start of the string forward
dynstr* dynstr_trim_start(dynstr* dstr, u32 amount);

bool dynstr_equals(dynstr* dstr, const char* other);