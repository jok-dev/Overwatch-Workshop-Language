#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include <stack>
#include <vector>

#include "tokenizer.h"
#include "dynstr.h"

// @Todo: we could add a printing of the parser intention stack on error?

enum ast_node_type
{
	AST_NODE_TYPE_END,

	AST_NODE_TYPE_NUMBER,
	AST_NODE_TYPE_STRING,
	AST_NODE_TYPE_IDENTIFIER,
	AST_NODE_TYPE_BOOLEAN,
	AST_NODE_TYPE_FUNCTION_CALL,
	AST_NODE_TYPE_IF,
	AST_NODE_TYPE_BINARY
};

struct ast_node;

struct ast_node_data_simple
{
	dynstr* value;
};

struct ast_node_data_bool
{
	bool value;
};

struct ast_node_data_function_call
{
	ast_node* function_identifier;

	u32 args_len;
	ast_node* args;
};

struct ast_node_data_if
{
	ast_node* cond;

	u32 then_body_statements_len;
	ast_node* then_body_statements;
};

struct ast_node_data_binary
{
	ast_node* left;
	ast_node* right;

	dynstr* op;
};

union ast_node_data
{
	ast_node_data_simple number;
	ast_node_data_simple string;

	ast_node_data_simple identifier;

	ast_node_data_bool boolean;

	ast_node_data_function_call fcall;

	ast_node_data_if iff;
	ast_node_data_binary binary;
};

struct ast_node
{
	ast_node_type type;
	ast_node_data data;

	// satements are individual lines
	bool statement = false;
};

std::stack<const char*> intention;

void parser_error(const char* error, ...)
{
	va_list args;
	va_start(args, error);

	printf("Parse error: ");
	vprintf(error, args);

	// @Todo: line support
	printf(" while %s at %i:%i\n", intention.top(), 0, tokenizer_get_pos());

	va_end(args);

	system("PAUSE");
	exit(0);
}

void parser_unexpected_token_error(token_type expected, const char* expected_value, const char* why_expected)
{
	token tok = token_peek();

	if(tok.type != TOKEN_TYPE_END)
	{
		parser_error("Unexpected token '%s', expected '%s' (to %s)", tok.value->raw, expected_value, why_expected);
	}
	else
	{
		parser_error("Unexpected %s, expected '%s' (to %s)", token_type_get_name(tok.type),
			expected_value, why_expected);
	}
}

bool node_requires_semicolon(ast_node node)
{
	return node.statement && node.type != AST_NODE_TYPE_IF && node.type != AST_NODE_TYPE_END;
}

// @Todo: invalid operators?
u32 operator_get_precedence(dynstr* op)
{
	if(dynstr_equals(op, "=")) return 1;
	if(dynstr_equals(op, "||")) return 2;
	if(dynstr_equals(op, "&&")) return 3;
	if(dynstr_equals(op, "<") || dynstr_equals(op, ">") || dynstr_equals(op, "<=") 
		|| dynstr_equals(op, ">=") || dynstr_equals(op, "==") || dynstr_equals(op, "!=")) return 7;
	if(dynstr_equals(op, "+") || dynstr_equals(op, "-")) return 10;
	if(dynstr_equals(op, "*") || dynstr_equals(op, "/") || dynstr_equals(op, "%") || dynstr_equals(op, "^")) return 20;
}

bool is_next_token(token_type type)
{
	token tok = token_peek();

	return tok.type == type;
}

bool is_next_token(token_type type, const char* value)
{
	token tok = token_peek();

	return tok.type == type && dynstr_equals(tok.value, value);
}

void eat_token(token_type type, const char* value, const char* why)
{
	if(!is_next_token(type, value)) parser_unexpected_token_error(type, value, why);

	token_next();
}

ast_node parse_expression();
ast_node parse_statement();

ast_node maybe_parse_function_call(ast_node node);

ast_node parse_if();
ast_node parse_bool();
ast_node parse_digit();
ast_node parse_string();
ast_node parse_identifier();
ast_node parse_function_call(ast_node identifier);

std::vector<ast_node> parse_delimited_list(const char* start_punc, const char* end_punc, const char* delimiter_punc, bool statment, const char* what)
{
	std::vector<ast_node> list;
	bool first = true;

	// @Todo: use ("start %s", what)   ???
	eat_token(TOKEN_TYPE_PUNCTUATION, start_punc, what);

	while(token_peek().type != TOKEN_TYPE_END) 
	{
		if(is_next_token(TOKEN_TYPE_PUNCTUATION, end_punc)) break;
		if(first) first = false; else if(!statment && strcmp(delimiter_punc, ";") != 0) eat_token(TOKEN_TYPE_PUNCTUATION, delimiter_punc, what);
		if(is_next_token(TOKEN_TYPE_PUNCTUATION, end_punc)) break; // the last separator can be missing

		list.push_back(statment ? parse_statement() : parse_expression());
	}

	eat_token(TOKEN_TYPE_PUNCTUATION, end_punc, what);

	return list;
}

ast_node parse_atom()
{
	// @Todo: allow people to make their own scopes?
	// @Todo: allow people to use () to change precedence?

	if(is_next_token(TOKEN_TYPE_KEYWORD, "if")) return parse_if();
	if(is_next_token(TOKEN_TYPE_KEYWORD, "true") || is_next_token(TOKEN_TYPE_KEYWORD, "false")) return parse_bool();

	token tok = token_peek();

	ast_node node = { AST_NODE_TYPE_END };

	if(tok.type == TOKEN_TYPE_END) return node;

	switch(tok.type)
	{
		case TOKEN_TYPE_DIGIT: node = parse_digit(); break;
		case TOKEN_TYPE_STRING: node = parse_string(); break;
		case TOKEN_TYPE_IDENTIFIER: node = parse_identifier(); break;

		default: parser_error("Unable to parse token '%s' of type '%s'", tok.value->raw, token_type_get_name(tok.type));
	}

	token_next();

	return maybe_parse_function_call(node);
}

ast_node maybe_parse_binary(ast_node left, u32 precedence)
{
	if(is_next_token(TOKEN_TYPE_OPERATOR))
	{
		token tok = token_peek();

		u32 other_prec = operator_get_precedence(tok.value);
		if(other_prec > precedence) {
			ast_node_data_binary binary_data;
			binary_data.op = dynstr_create(tok.value);

			token_next();

			ast_node right = maybe_parse_binary(parse_atom(), other_prec);

			ast_node* left_pointer = (ast_node*) malloc(sizeof(ast_node));
			*left_pointer = left;
			binary_data.left = left_pointer;

			ast_node* right_pointer = (ast_node*) malloc(sizeof(ast_node));
			*right_pointer = right;
			binary_data.right = right_pointer;

			ast_node node = {};
			node.type = AST_NODE_TYPE_BINARY;
			node.data.binary = binary_data;

			return maybe_parse_binary(node, precedence);
		}
	}

	return left;
}

ast_node maybe_parse_function_call(ast_node node)
{
	if(is_next_token(TOKEN_TYPE_PUNCTUATION, "(")) return parse_function_call(node);
	
	return node;
}

ast_node parse_expression()
{
	intention.push("parsing expression");
	ast_node node = maybe_parse_function_call(maybe_parse_binary(parse_atom(), 0));
	intention.pop();
	return node;
}

// statements are individual "lines", they require a ';' finish, unlike expressions that could be in if conditions for example
// (unless they are an if)
ast_node parse_statement()
{
	intention.push("parsing statement");

	ast_node node = parse_expression();
	node.statement = true;

	if(node_requires_semicolon(node))
	{
		eat_token(TOKEN_TYPE_PUNCTUATION, ";", "end statement");
	}

	intention.pop();
	return node;
}

ast_node parse_if()
{
	intention.push("parsing if statement");

	ast_node_data_if data;

	eat_token(TOKEN_TYPE_KEYWORD, "if", "start if statement");

	eat_token(TOKEN_TYPE_PUNCTUATION, "(", "start if condition");

	// @Todo: this is kinda bad, we never free either?
	ast_node* cond = (ast_node*) malloc(sizeof(ast_node));

	// @Todo: instead of this, maybe pass this on to the expression parser?
	intention.push("parsing if condition");
	*cond = parse_expression();
	intention.pop();

	data.cond = cond;

	eat_token(TOKEN_TYPE_PUNCTUATION, ")", "end if condition");

	std::vector<ast_node> list = parse_delimited_list("{", "}", ";", true, "if-then body");
	ast_node* statements = NULL;

	if(list.size() > 0)
	{
		statements = (ast_node*) malloc(sizeof(ast_node) * list.size());

		for(u32 i = 0; i < list.size(); i++)
		{
			statements[i] = list[i];
		}
	}

	data.then_body_statements = statements;
	data.then_body_statements_len = list.size();

	ast_node node = {};
	node.type = AST_NODE_TYPE_IF;
	node.data.iff = data;

	intention.pop();
	return node;
}

ast_node parse_bool()
{
	intention.push("parsing bool keyword");

	ast_node_data_bool data = {};

	if(is_next_token(TOKEN_TYPE_KEYWORD, "true")) data.value = true;
	else if(is_next_token(TOKEN_TYPE_KEYWORD, "false")) data.value = false;
	else assert("Unsupported boolean keyword!");

	ast_node node = {};
	node.type = AST_NODE_TYPE_BOOLEAN;
	node.data.boolean = data;

	token_next();

	intention.pop();
	return node;
}

ast_node parse_digit()
{
	intention.push("parsing digit");

	token tok = token_peek();

	ast_node node = {};

	node.type = AST_NODE_TYPE_NUMBER;

	ast_node_data_simple data;
	data.value = dynstr_create(tok.value);

	node.data.number = data;

	intention.pop();
	return node;
}

ast_node parse_string()
{
	intention.push("parsing string");

	token tok = token_peek();

	ast_node_data_simple data = {};
	data.value = dynstr_create(tok.value);

	ast_node node = {};
	node.type = AST_NODE_TYPE_STRING;
	node.data.string = data;

	intention.pop();
	return node;
}

ast_node parse_identifier()
{
	intention.push("parsing identifier");

	token tok = token_peek();

	ast_node node = {};
	node.type = AST_NODE_TYPE_IDENTIFIER;

	ast_node_data_simple data = {};
	data.value = dynstr_create(tok.value);

	node.data.identifier = data;

	intention.pop();
	return node;
}

ast_node parse_function_call(ast_node identifier)
{
	intention.push("parsing function call");

	token tok = token_peek();

	ast_node node = {};
	node.type = AST_NODE_TYPE_FUNCTION_CALL;

	ast_node_data_function_call data = {};
	
	ast_node* id_pointer = (ast_node*) malloc(sizeof(ast_node));
	*id_pointer = identifier;
	data.function_identifier = id_pointer;

	intention.push("parse function arguments");
	std::vector<ast_node> list = parse_delimited_list("(", ")", ",", false, "function args");

	ast_node* args = NULL;
	
	if(list.size() > 0)
	{
		args = (ast_node*) malloc(sizeof(ast_node) * list.size());

		for(u32 i = 0; i < list.size(); i++)
		{
			args[i] = list[i];
		}
	}

	data.args = args;
	data.args_len = list.size();

	intention.pop();

	node.data.fcall = data;

	intention.pop();
	return node;
}

dynstr* stringify_ast(ast_node node)
{
	dynstr* str = dynstr_create(20);

	switch(node.type)
	{
		case AST_NODE_TYPE_NUMBER:
		{
			dynstr_append(str, "number(%s)", node.data.number.value->raw);
		} break;

		case AST_NODE_TYPE_STRING:
		{
			dynstr_append(str, "\"%s\"", node.data.string.value->raw);
		} break;

		case AST_NODE_TYPE_BOOLEAN:
		{
			dynstr_append_str(str, node.data.boolean.value ? "true" : "false");
		} break;

		case AST_NODE_TYPE_IF:
		{
			dynstr* cond = stringify_ast(*node.data.iff.cond);
			dynstr_append(str, "if(%s)\n{\n", cond->raw);

			for(u32 i = 0; i < node.data.iff.then_body_statements_len; i++)
			{
				dynstr* arg = stringify_ast(node.data.iff.then_body_statements[i]);
				dynstr_append_str(str, "    ");
				dynstr_append(str, arg);
				dynstr_destroy(arg);
			}

			dynstr_append(str, "}\n");
			dynstr_destroy(cond);
		} break;

		case AST_NODE_TYPE_BINARY:
		{
			dynstr* left = stringify_ast(*node.data.binary.left);
			dynstr* right = stringify_ast(*node.data.binary.right);

			dynstr_append(str, "%s %s %s", left->raw, node.data.binary.op->raw, right->raw);

			dynstr_destroy(left);
			dynstr_destroy(right);
		} break;

		case AST_NODE_TYPE_IDENTIFIER:
		{
			dynstr_append(str, node.data.identifier.value);
		} break;

		case AST_NODE_TYPE_FUNCTION_CALL:
		{
			dynstr* fid = stringify_ast(*node.data.fcall.function_identifier);

			dynstr_append(str, "%s(", fid->raw);

			for(u32 i = 0; i < node.data.fcall.args_len; i++)
			{
				dynstr* arg = stringify_ast(node.data.fcall.args[i]);
				dynstr_append(str, arg);
				dynstr_destroy(arg);

				if(i + 1 < node.data.fcall.args_len) dynstr_append_str(str, ", ");
			}

			dynstr_append_char(str, ')');

			dynstr_destroy(fid);
		} break;
	}

	if(node_requires_semicolon(node))
	{
		dynstr_append(str, ";\n");
	}

	return str;
}

int main()
{
	tokenizer_init("if(this == true) { test(6969); }");

	ast_node node;
	while((node = parse_statement()).type != AST_NODE_TYPE_END)
	{
		dynstr* str = stringify_ast(node);
		printf("%s", str->raw);
		dynstr_destroy(str);
	}

	printf("\n");

	system("PAUSE");
	exit(0);
}