#pragma once

#include "dynstr.h"

typedef enum token_type
{
	TOKEN_TYPE_END,

	TOKEN_TYPE_STRING,
	TOKEN_TYPE_DIGIT,
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_PUNCTUATION,
	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_IDENTIFIER
} token_type_t;

struct token
{
	token_type type;
	dynstr* value;
};

void tokenizer_init(const char* code_lines);

const char* token_type_get_name(token_type type);

token token_next();
token token_peek();

u64 tokenizer_get_current_line();
u64 tokenizer_get_current_line_pos();