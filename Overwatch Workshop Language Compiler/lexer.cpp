#include "lexer.h"

#include <string.h>

#include <ctype.h>

#include "dynstr.h"

const char operators[] = { '+', '-', '*', '/', '%', '^', '&', '|', '!', '>', '<', '=' };

const char punctuation[] = { '(', ')', '{', '}', ',', ';', '"', '.' };

const char* keywords[] = { "if", "event", "true", "false" };

const char* code;

u64 code_len;

u64 pos = 0;
u64 current_line_start_pos = 0;
u64 current_line = 1;

char current_char;

bool has_current_token = false;
token current_token;

// tokens stay in memory for 1 token_next call
bool has_last_token = false;
token last_token;

// @Todo: move to charstream??
bool char_end()
{
	return pos >= code_len;
}

bool char_end_further()
{
	return pos + 1 >= code_len;
}

char char_peek()
{
	return code[pos];
}

char char_peek_further()
{
	return code[pos + 1];
}

char char_next()
{
	return current_char = code[pos++];
}

void lexer_init(const char* code_lines)
{
	code = code_lines;
	code_len = strlen(code);
	pos = 0;
}

const char* token_type_get_name(token_type type)
{
	switch(type)
	{
		case TOKEN_TYPE_END: return "end";
		case TOKEN_TYPE_STRING: return "string";
		case TOKEN_TYPE_DIGIT: return "digit";
		case TOKEN_TYPE_OPERATOR: return "operator";
		case TOKEN_TYPE_PUNCTUATION: return "punctuation";
		case TOKEN_TYPE_KEYWORD: return "keyword";
		case TOKEN_TYPE_IDENTIFIER: return "identifier";
	}

	assert("Unkown token type when getting type name");
}

u32 token_get_length(token tok)
{
	token_type type = tok.type;

	if(type == TOKEN_TYPE_END) return 0;

	return tok.value->len;
}

u64 lexer_get_current_line()
{
	return current_line;
}

u64 lexer_get_current_line_pos()
{
	return pos - current_line_start_pos;
}

// token character type checks
bool is_digit_start_char(char ch)
{
	return isdigit(ch);
}

bool is_digit_char(char ch)
{
	return isdigit(ch) || ch == '.';
}

bool is_operator_char(char ch)
{
	for(u32 i = 0; i < stack_array_length(operators); i++) if(operators[i] == ch) return true;

	return false;
}

bool is_punctuation_char(char ch)
{
	for(u32 i = 0; i < stack_array_length(punctuation); i++) if(punctuation[i] == ch) return true;

	return false;
}

bool is_identifier_start_char(char ch)
{
	// identifiers can start with anything apart from digits, operators or punctuation or space
	return isalpha(ch) || (!is_digit_start_char(ch) && !is_operator_char(ch) && !is_punctuation_char(ch) && ch != ' ');
}

bool is_identifier_char(char ch)
{
	return is_identifier_start_char(ch) || is_digit_start_char(ch);
}

bool is_identifier_keyword(dynstr* iden)
{
	for(u32 i = 0; i < stack_array_length(keywords); i++) if(dynstr_equals(iden, keywords[i])) return true;

	return false;
}

token token_read_string()
{
	// @Speed: we could write our own allocator for this to be way quicker, but I doubt compile times will be the longest thing about this process..
	dynstr* str = dynstr_create(20);
	bool escaped = false;

	char_next();

	while(!char_end())
	{
		char ch = char_next();
		if(escaped)
		{
			escaped = false;
		}
		else if(ch == '\\')
		{
			escaped = true;
			continue;
		}
		else if(ch == '\"')
		{
			break;
		}

		dynstr_append_char(str, ch);
	}

	token tok = {};

	tok.type = TOKEN_TYPE_STRING;
	tok.value = str;

	return tok;
}

token token_read_digit()
{
	// @Speed: we could write our own allocator for this to be way quicker, but I doubt compile times will be the longest thing about this process..
	dynstr* digit = dynstr_create(20);
	while(!char_end() && is_digit_char(char_peek()))
	{
		dynstr_append_char(digit, char_next());
	}

	token tok = {};

	// @Todo: check format?

	tok.type = TOKEN_TYPE_DIGIT;
	tok.value = digit;

	return tok;
}

token token_read_operator()
{
	// @Speed: we could write our own allocator for this to be way quicker, but I doubt compile times will be the longest thing about this process..
	dynstr* op = dynstr_create(20);
	while(!char_end() && is_operator_char(char_peek()))
	{
		dynstr_append_char(op, char_next());
	}

	token tok = {};

	tok.type = TOKEN_TYPE_OPERATOR;
	tok.value = op;

	return tok;
}

token token_read_punctuation()
{
	token tok = {};

	// @Todo: what about function support??
	// if it's not a keyword, it must be a variable name
	tok.type = TOKEN_TYPE_PUNCTUATION;
	tok.value = dynstr_create(1);
	dynstr_append_char(tok.value, char_next());

	return tok;
}

token token_read_identifier()
{
	// @Speed: we could write our own allocator for this to be way quicker, but I doubt compile times will be the longest thing about this process..
	dynstr* iden = dynstr_create(20);
	while(!char_end() && is_identifier_char(char_peek()))
	{
		dynstr_append_char(iden, char_next());
	}

	token tok = {};

	// if it's not a keyword, it must be a variable name
	if(is_identifier_keyword(iden))
	{
		tok.type = TOKEN_TYPE_KEYWORD;
	}
	else
	{
		tok.type = TOKEN_TYPE_IDENTIFIER;
	}

	tok.value = iden;

	return tok;
}

token token_next_internal()
{
	// eat whitespace
	while(!char_end() && (char_peek() == ' ' || char_peek() == '\n' || char_peek() == '\t'))
	{
		if(char_peek() == '\n')
		{
			current_line_start_pos = pos;
			current_line++;
		}

		char_next();
	}

	// no token to read because we're at the end of the code
	if(char_end()) return { TOKEN_TYPE_END };

	char ch = char_peek();

	// comment
	if(char_peek() == '/' && !char_end_further() && char_peek_further() == '/')
	{
		// skip till newline
		while(!char_end() && char_next() != '\n');

		current_line++;

		return token_next_internal();
	}

	if(ch == '"') return token_read_string();
	if(is_digit_start_char(ch)) return token_read_digit();
	if(is_identifier_start_char(ch)) return token_read_identifier();
	if(is_punctuation_char(ch)) return token_read_punctuation();
	if(is_operator_char(ch)) return token_read_operator();

	// @Todo: better errors
	assert(false && "Failed to parse character");
}

// @Todo: move up like we did for parser?
token token_next()
{
	if(has_last_token && last_token.type != TOKEN_TYPE_END)
	{
		dynstr_destroy(last_token.value);
	}

	token tok = token_next_internal();

	has_last_token = has_current_token;
	if(has_last_token) last_token = current_token;

	has_current_token = true;
	current_token = tok;
	return tok;
}

token token_peek()
{
	if(has_current_token)
	{
		return current_token;
	}

	token_next();
	return current_token;
}