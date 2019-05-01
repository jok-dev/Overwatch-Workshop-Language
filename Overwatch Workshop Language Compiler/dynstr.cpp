#include "dynstr.h"

dynstr* dynstr_create(char* str, u32 str_len, u32 buf_len)
{
	assert(str_len >= 0 && buf_len >= 0 && "You can't have a negatively sized string or buffer");

	dynstr* mem = (dynstr*) malloc(sizeof(dynstr));
	char* raw = (char*) malloc(buf_len + 1);

	dynstr* dstr = (dynstr*) mem;
	dstr->raw = raw;
	dstr->len = str_len;
	dstr->buf_len = buf_len;

	if (str != 0)
	{
		strncpy(raw, str, str_len);
	}

	// add the nullterm
	*(raw + str_len) = 0;

	return dstr;
}

dynstr* dynstr_create(char* str, u32 len)
{
	assert(len <= UINT16_MAX && "Dynstr only supports strings of length <= 65535");

	return dynstr_create(str, (u16)len, (u16)len);
}

dynstr* dynstr_create(char* str)
{
	size_t len = strlen(str);
	assert(len <= UINT16_MAX && "Dynstr only supports strings of length <= 65535");

	return dynstr_create(str, (u16)len, (u16)len);
}

dynstr* dynstr_create(u32 buf_len)
{
	return dynstr_create(0, 0, buf_len);
}

dynstr* dynstr_create()
{
	return dynstr_create(0, 0, 0);
}

void dynstr_destroy(dynstr* str)
{
	free(str->raw);
	free(str);
}

dynstr* dynstr_set_buflen(dynstr* dstr, u32 new_buf_len)
{
	dstr->raw = (char*)realloc(dstr->raw, new_buf_len + 1);
	dstr->buf_len = new_buf_len;

	return dstr;
}

dynstr* dynstr_set_strlen(dynstr* dstr, u32 str_len)
{
	dstr->len = str_len;
	dstr->raw[str_len] = 0;

	return dstr;
}

// Ensures that the capacity is at least equal to the specified minimum. If the capacity is smaller it will be expanded to the maximum of: min_len. buf_len * 2 + 2
dynstr* dynstr_ensure_buflen(dynstr* dstr, u32 min_len)
{
	assert(min_len <= UINT16_MAX && "Dynstr only supports strings of length <= 65535");

	if (dstr->buf_len < min_len)
	{
		dynstr_set_buflen(dstr, max(min_len, dstr->buf_len * 2 + 2));
	}

	return dstr;
}

dynstr* dynstr_append_char(dynstr* to, char from)
{
	u32 new_len = to->len + 1;
	dynstr_ensure_buflen(to, new_len);

	to->raw[to->len] = from;

	dynstr_set_strlen(to, new_len);

	return to;
}

// The dynstr pointer will still be valid, but the raw pointer might be invalidated
dynstr* dynstr_append_str(dynstr* to, char* from, u32 from_len)
{
	assert(from != 0 && "Cannot append null string");

	if (from_len == 0)
	{
		return to;
	}

	u32 old_len = to->len;
	u32 new_len = old_len + from_len;

	dynstr_ensure_buflen(to, new_len);

	strncpy(to->raw + old_len, from, from_len);

	dynstr_set_strlen(to, new_len);

	return to;
}

dynstr* dynstr_append_str(dynstr* to, char* from)
{
	size_t from_len = strlen(from);

	return dynstr_append_str(to, from, from_len);
}

dynstr* dynstr_append_int(dynstr* to, int from)
{
	// @Speed: slow :(
	u32 length = snprintf(NULL, 0, "%d", from);
	char* str = (char*)malloc(length + 1);
	snprintf(str, length + 1, "%d", from);

	dynstr_append_str(to, str, length);
	free(str);

	return to;
}

dynstr* dynstr_append_va(dynstr* to, char* format, va_list args)
{
	u32 arg_index = 0;

	bool last_was_percent = false;
	for (u32 i = 0; true; i++)
	{
		char c = format[i];
		if (c == 0) break;

		if (last_was_percent)
		{
			if (c == 's')
			{
				dynstr_append_str(to, va_arg(args, char*));
			}
			else if (c == 'i')
			{
				int integer = va_arg(args, int);
				dynstr_append_int(to, integer);
			}
			else if (c == '%')
			{
				dynstr_append_char(to, '%');
			}

			last_was_percent = false;
			continue;
		}

		if (c != '%')
		{
			dynstr_append_char(to, c);
		}
		else
		{
			last_was_percent = true;
		}
	}

	return to;
}

dynstr* dynstr_append(dynstr* to, char* format, ...)
{
	va_list args;
	va_start(args, format);

	dynstr_append_va(to, format, args);

	va_end(args);

	return to;
}

dynstr* dynstr_append(dynstr* to, dynstr* from)
{
	dynstr_append_str(to, from->raw, from->len);
	return to;
}

dynstr* dynstr_clear(dynstr* dstr)
{
	dstr->len = 0;
	dstr->raw[0] = 0;

	return dstr;
}

dynstr* dynstr_trim_start(dynstr* dstr, u32 amount)
{
	assert(amount < dstr->len && "You cannot trim further than 1 minus the len of the string");

	u32 len = dstr->len - amount;
	memcpy(dstr->raw, dstr->raw + amount, len);
	dynstr_set_strlen(dstr, len);

	return dstr;
}

bool dynstr_equals(dynstr* dstr, char* other)
{
	return strcmp(dstr->raw, other) == 0;
}