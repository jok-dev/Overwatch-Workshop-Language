#include "parser.h"

#include "dynstr.h"
#include "lexer.h"
#include "ast.h"

#include <assert.h>
#include <stdarg.h>

#include <stack>
#include <vector>

std::stack<const char*> intention;

void parser_error(const char* error, ...)
{
	va_list args;
	va_start(args, error);

	printf("Parse error: ");
	vprintf(error, args);

	// @Todo: line support
	printf(" while %s at %i:%i\n", intention.top(), lexer_get_current_line(), lexer_get_current_line_pos());

	va_end(args);

	system("PAUSE");
	exit(0);
}

void parser_unexpected_token_error(token_type expected, const char* expected_value, bool has_value, const char* why_expected, va_list why_expected_args)
{
	token tok = token_peek();

	dynstr* why_expected_formatted = dynstr_create(30);
	dynstr_append_va(why_expected_formatted, why_expected, why_expected_args);

	dynstr* expected_formatted = dynstr_create(30);
	dynstr_append(expected_formatted, has_value ? "'%s'" : "%s", has_value ? expected_value : token_type_get_name(expected));

	if(tok.type != TOKEN_TYPE_END && tok.type != TOKEN_TYPE_STRING)
	{
		parser_error("Unexpected token '%s', expected %s (to %s)", tok.value->raw, expected_formatted->raw, why_expected_formatted->raw);
	}
	else
	{
		parser_error("Unexpected %s, expected %s (to %s)", token_type_get_name(tok.type),
			expected_formatted->raw, why_expected_formatted->raw);
	}

	dynstr_destroy(why_expected_formatted);
	dynstr_destroy(expected_formatted);
}

void parser_unexpected_token_error(token_type expected, const char* expected_value, const char* why_expected, va_list why_expected_args)
{
	parser_unexpected_token_error(expected, expected_value, true, why_expected, why_expected_args);
}

void parser_unexpected_token_error(token_type expected, const char* why_expected, va_list why_expected_args)
{
	parser_unexpected_token_error(expected, 0, false, why_expected, why_expected_args);
}

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

token eat_token(token_type type, const char* value, const char* why, ...)
{
	va_list args;
	va_start(args, why);

	if(!is_next_token(type, value)) parser_unexpected_token_error(type, value, why, args);

	token tok = token_peek();
	token_next();

	va_end(args);

	return tok;
}

token eat_token(token_type type, const char* why, ...)
{
	va_list args;
	va_start(args, why);

	if(!is_next_token(type)) parser_unexpected_token_error(type, why, args);

	token tok = token_peek();
	token_next();

	va_end(args);

	return tok;
}

ast_node parse_expression();
ast_node parse_statement();

ast_node maybe_parse_extras(ast_node node);

ast_node parse_if();
ast_node parse_event();
ast_node parse_bool();
ast_node parse_digit(token tok);
ast_node parse_string(token tok);
ast_node parse_identifier(token tok);
ast_node parse_floating_operator(token tok);
ast_node parse_member_access(ast_node identifier);
ast_node parse_function_call(ast_node identifier);

std::vector<ast_node> parse_delimited_list(const char* start_punc, const char* end_punc, const char* delimiter_punc, bool statment, const char* what)
{
	std::vector<ast_node> list;
	bool first = true;

	eat_token(TOKEN_TYPE_PUNCTUATION, start_punc, "start %s", what);

	while(token_peek().type != TOKEN_TYPE_END)
	{
		if(is_next_token(TOKEN_TYPE_PUNCTUATION, end_punc)) break;
		if(first) first = false; else if(!statment && strcmp(delimiter_punc, ";") != 0) eat_token(TOKEN_TYPE_PUNCTUATION, delimiter_punc, "give next %s", what);
		if(is_next_token(TOKEN_TYPE_PUNCTUATION, end_punc)) break; // the last separator can be missing

		list.push_back(statment ? parse_statement() : parse_expression());
	}

	eat_token(TOKEN_TYPE_PUNCTUATION, end_punc, "end %s", what);

	return list;
}

ast_node* ast_node_list_to_array(std::vector<ast_node> list)
{
	ast_node* arr = NULL;

	if(list.size() > 0)
	{
		arr = (ast_node*)malloc(sizeof(ast_node) * list.size());

		for(u32 i = 0; i < list.size(); i++)
		{
			arr[i] = list[i];
		}
	}

	return arr;
}

ast_node parse_atom()
{
	// @Todo: allow people to make their own scopes?
	// @Todo: allow people to use () to change precedence?

	if(is_next_token(TOKEN_TYPE_KEYWORD, "if")) return parse_if();
	if(is_next_token(TOKEN_TYPE_KEYWORD, "event")) return parse_event();
	if(is_next_token(TOKEN_TYPE_KEYWORD, "true") || is_next_token(TOKEN_TYPE_KEYWORD, "false")) return parse_bool();

	token tok = token_peek();

	ast_node node = { };
	node.type = AST_NODE_TYPE_END;

	if(tok.type == TOKEN_TYPE_END) return node;

	switch(tok.type)
	{
		case TOKEN_TYPE_DIGIT: node = parse_digit(tok); break;
		case TOKEN_TYPE_STRING: node = parse_string(tok); break;
		case TOKEN_TYPE_IDENTIFIER: node = parse_identifier(tok); break;
		case TOKEN_TYPE_OPERATOR: node = parse_floating_operator(tok); break;

		default: parser_error("Unable to parse token '%s' of type '%s'", tok.value->raw, token_type_get_name(tok.type));
	}

	return maybe_parse_extras(node);
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

			ast_node* left_pointer = (ast_node*)malloc(sizeof(ast_node));
			*left_pointer = left;
			binary_data.left = left_pointer;

			ast_node* right_pointer = (ast_node*)malloc(sizeof(ast_node));
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

ast_node maybe_parse_member_access(ast_node node)
{
	if(is_next_token(TOKEN_TYPE_PUNCTUATION, ".")) return parse_member_access(node);

	return node;
}

ast_node maybe_parse_function_call(ast_node node)
{
	if(is_next_token(TOKEN_TYPE_PUNCTUATION, "(")) return parse_function_call(node);

	return node;
}

ast_node maybe_parse_extras(ast_node node)
{
	return maybe_parse_member_access(maybe_parse_function_call(node));
}

ast_node parse_expression()
{
	intention.push("parsing expression");
	ast_node node = maybe_parse_extras(maybe_parse_binary(parse_atom(), 0));
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

	if(ast_node_requires_semicolon(node))
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

	data.then_body_statements = ast_node_list_to_array(list);
	data.then_body_statements_len = list.size();

	ast_node node = {};
	node.type = AST_NODE_TYPE_IF;
	node.data.if_statement = data;

	intention.pop();
	return node;
}

ast_node parse_event()
{
	intention.push("parsing event statement");

	ast_node node = {};
	node.type = AST_NODE_TYPE_EVENT;

	eat_token(TOKEN_TYPE_KEYWORD, "event", "start event statement");

	std::vector<ast_node> args_list = parse_delimited_list("(", ")", ",", false, "event arguments");

	node.data.event_statement.args = ast_node_list_to_array(args_list);
	node.data.event_statement.args_len = args_list.size();

	std::vector<ast_node> body_list = parse_delimited_list("{", "}", ";", true, "event body");

	node.data.event_statement.body_statements = ast_node_list_to_array(body_list);
	node.data.event_statement.body_statements_len = body_list.size();

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

ast_node parse_digit(token tok)
{
	intention.push("parsing digit");

	ast_node node = {};

	node.type = AST_NODE_TYPE_NUMBER;

	ast_node_data_simple data;
	data.value = dynstr_create(tok.value);

	node.data.number = data;

	token_next();

	intention.pop();
	return node;
}

ast_node parse_string(token tok)
{
	intention.push("parsing string");

	ast_node_data_simple data = {};
	data.value = dynstr_create(tok.value);

	ast_node node = {};
	node.type = AST_NODE_TYPE_STRING;
	node.data.string = data;

	token_next();

	intention.pop();
	return node;
}

ast_node parse_identifier(token tok)
{
	intention.push("parsing identifier");

	ast_node node = {};
	node.type = AST_NODE_TYPE_IDENTIFIER;

	ast_node_data_simple data = {};
	data.value = dynstr_create(tok.value);

	node.data.identifier = data;

	token_next();

	intention.pop();
	return node;
}

ast_node parse_floating_operator(token tok)
{
	intention.push("parsing floating operator");

	ast_node node = {};
	node.type = AST_NODE_TYPE_FLOATING_OPERATOR;

	ast_node_data_floating_operator data = {};
	data.op = dynstr_create(tok.value);

	token_next();

	ast_node* child = (ast_node*) malloc(sizeof(ast_node));
	*child = parse_expression();
	data.child = child;

	node.data.floating_operator = data;

	intention.pop();
	return node;
}

ast_node parse_member_access(ast_node owner)
{
	intention.push("parsing member access");

	ast_node node = {};
	node.type = AST_NODE_TYPE_MEMBER_ACCESS;

	eat_token(TOKEN_TYPE_PUNCTUATION, ".", "access member");

	ast_node_data_member_access data = {};
	ast_node* owner_pointer = (ast_node*)malloc(sizeof(ast_node));
	*owner_pointer = owner;
	data.owner = owner_pointer;

	ast_node* member_pointer = (ast_node*) malloc(sizeof(ast_node));
	*member_pointer = parse_expression();
	data.member = member_pointer;

	node.data.member_access = data;

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

	std::vector<ast_node> list = parse_delimited_list("(", ")", ",", false, "function args");

	data.args = ast_node_list_to_array(list);
	data.args_len = list.size();

	node.data.fcall = data;

	intention.pop();
	return node;
}