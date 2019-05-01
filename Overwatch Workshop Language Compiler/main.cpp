#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <ctype.h>

#include "tokenizer.cpp"
#include "dynstr.h"


/*
	An identifier is 
*/

const char* code = "if(is_game_in_progress() == true) { big_message(event_player, \"chasing\"); }";

unsigned long code_len = strlen(code);
unsigned long pos = 0;
char current_char;

bool char_end()
{
	return pos >= code_len;
}

char char_peek()
{
	return code[pos];
}

char char_next()
{
	return current_char = code[pos++];
}

typedef enum token_type
{
	TOKEN_TYPE_END,

	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_VARIABLE
} token_type_t;

struct token
{
	token_type type;

	void* value;
};

// token character type checks
bool is_identifier_start_char(char ch)
{
	return isalpha(ch);
}

bool is_identifier_char(char ch)
{
	return isalpha(ch) || isdigit(ch);
}

bool is_identifier_keyword(dynstr* iden)
{
	return dynstr_equals(iden, "if") || dynstr_equals(iden, "equals");
}

token token_read_identifier();

token token_next()
{
	// eat whitespace
	while (!char_end() && char_peek() == ' ') char_next();

	// no token to read because we're at the end of the code
	if (char_end()) return { TOKEN_TYPE_END };

	char ch = char_peek();

	return token_read_identifier();

	// @Todo: better errors
	assert(false && "Failed to parse character");
}

token token_read_identifier()
{
	// @Speed: we could write our own allocator for this to be way quicker, but I doubt compile times will be the longest thing about this process..
	dynstr* iden = dynstr_create(20);
	while (!char_end() && is_identifier_char(char_peek()))
	{
		dynstr_append_char(iden, char_next());
	}

	token tok = {};

	// @Todo: what about function support??
	// if it's not a keyword, it must be a variable name
	if(is_identifier_keyword(iden)) tok.type = TOKEN_TYPE_KEYWORD;
	else tok.type == TOKEN_TYPE_VARIABLE;

	tok.value = iden;

	return tok;
}

int main()
{
	token tok;
	while (true)
	{
		tok = token_next();

		if (tok.type == TOKEN_TYPE_KEYWORD)
		{
			dynstr* dstr = (dynstr*) tok.value;
			printf("%s\n", dstr->raw);
		}

		if (tok.type == TOKEN_TYPE_END) break;
	}

	printf("\n");

	system("PAUSE");
}