#pragma once

#include "dynstr.h"

enum ast_node_type
{
	AST_NODE_TYPE_END,

	AST_NODE_TYPE_NUMBER,
	AST_NODE_TYPE_STRING,
	AST_NODE_TYPE_IDENTIFIER,
	AST_NODE_TYPE_FLOATING_OPERATOR,
	AST_NODE_TYPE_BOOLEAN,
	AST_NODE_TYPE_FUNCTION_CALL,
	AST_NODE_TYPE_IF,
	AST_NODE_TYPE_EVENT,
	AST_NODE_TYPE_BINARY,
	AST_NODE_TYPE_MEMBER_ACCESS,
};

struct ast_node;

struct ast_node_data_simple
{
	dynstr* value;
};

struct ast_node_data_floating_operator
{
	dynstr* op;

	ast_node* child;
};

struct ast_node_data_bool
{
	bool value;
};

struct ast_node_data_member_access
{
	ast_node* owner;
	ast_node* member;
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

struct ast_node_data_event
{
	u32 args_len;
	ast_node* args;

	u32 body_statements_len;
	ast_node* body_statements;
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

	ast_node_data_floating_operator floating_operator;

	ast_node_data_bool boolean;

	ast_node_data_member_access member_access;
	ast_node_data_function_call fcall;

	ast_node_data_event event_statement;
	ast_node_data_if if_statement;

	ast_node_data_binary binary;
};

struct ast_node
{
	ast_node_type type;
	ast_node_data data;

	// satements are individual lines
	bool statement = false;
};

dynstr* ast_stringify_as_code(ast_node node);
dynstr* ast_stringify_as_tree(ast_node node);

bool ast_node_requires_semicolon(ast_node node);