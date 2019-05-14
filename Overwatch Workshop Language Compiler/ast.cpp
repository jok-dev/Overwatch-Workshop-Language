#include "ast.h"

bool ast_node_requires_semicolon(ast_node node)
{
	return node.statement && node.type != AST_NODE_TYPE_IF && node.type != AST_NODE_TYPE_EVENT && node.type != AST_NODE_TYPE_END;
}

dynstr* ast_stringify_as_code(ast_node node, u32 depth)
{
	dynstr* str = dynstr_create(20);

	switch(node.type)
	{
	case AST_NODE_TYPE_NUMBER:
	{
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "%s", node.data.number.value->raw);
	} break;

	case AST_NODE_TYPE_STRING:
	{
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "\"%s\"", node.data.string.value->raw);
	} break;

	case AST_NODE_TYPE_BOOLEAN:
	{
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append_str(str, node.data.boolean.value ? "true" : "false");
	} break;

	case AST_NODE_TYPE_IF:
	{
		dynstr* cond = ast_stringify_as_code(*node.data.if_statement.cond, 0);

		dynstr_append_char(str, '\n');
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "if(%s)\n", cond->raw);
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "{\n");

		for(u32 i = 0; i < node.data.if_statement.then_body_statements_len; i++)
		{
			dynstr* arg = ast_stringify_as_code(node.data.if_statement.then_body_statements[i], depth + 1);
			dynstr_append(str, arg);
			dynstr_destroy(arg);
		}

		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "}\n");
		dynstr_destroy(cond);
	} break;

	case AST_NODE_TYPE_EVENT:
	{
		dynstr_append(str, "event(");

		for(u32 i = 0; i < node.data.event_statement.args_len; i++)
		{
			dynstr* arg = ast_stringify_as_code(node.data.event_statement.args[i], 0);
			dynstr_append_char_repeat(str, ' ', depth * 4);
			dynstr_append(str, arg);
			dynstr_destroy(arg);

			if(i + 1 < node.data.event_statement.args_len) dynstr_append_str(str, ", ");
		}

		dynstr_append(str, ") {\n");

		for(u32 i = 0; i < node.data.event_statement.body_statements_len; i++)
		{
			dynstr* arg = ast_stringify_as_code(node.data.event_statement.body_statements[i], depth + 1);
			dynstr_append(str, arg);
			dynstr_destroy(arg);
		}

		dynstr_append(str, "}\n");
	} break;

	case AST_NODE_TYPE_BINARY:
	{
		dynstr* left = ast_stringify_as_code(*node.data.binary.left, 0);
		dynstr* right = ast_stringify_as_code(*node.data.binary.right, 0);

		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "%s %s %s", left->raw, node.data.binary.op->raw, right->raw);

		dynstr_destroy(left);
		dynstr_destroy(right);
	} break;

	case AST_NODE_TYPE_IDENTIFIER:
	{
		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, node.data.identifier.value);
	} break;

	case AST_NODE_TYPE_MEMBER_ACCESS:
	{
		dynstr* owner = ast_stringify_as_code(*node.data.member_access.owner, 0);
		dynstr* member = ast_stringify_as_code(*node.data.member_access.member, 0);

		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "%s.%s", owner->raw, member->raw);
	} break;

	case AST_NODE_TYPE_FUNCTION_CALL:
	{
		dynstr* fid = ast_stringify_as_code(*node.data.fcall.function_identifier, 0);

		dynstr_append_char_repeat(str, ' ', depth * 4);
		dynstr_append(str, "%s(", fid->raw);

		for(u32 i = 0; i < node.data.fcall.args_len; i++)
		{
			dynstr* arg = ast_stringify_as_code(node.data.fcall.args[i], 0);
			dynstr_append(str, arg);
			dynstr_destroy(arg);

			if(i + 1 < node.data.fcall.args_len) dynstr_append_str(str, ", ");
		}

		dynstr_append_char(str, ')');

		dynstr_destroy(fid);
	} break;
	}

	if(ast_node_requires_semicolon(node))
	{
		dynstr_append(str, ";\n");
	}

	return str;
}

dynstr* ast_stringify_as_code(ast_node node)
{
	return ast_stringify_as_code(node, 0);
}

dynstr* ast_stringify_as_tree(ast_node node, u32 depth)
{
	dynstr* str = dynstr_create(20);

	switch(node.type)
	{
	case AST_NODE_TYPE_NUMBER:
	{
		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "number %s", node.data.number.value->raw);
	} break;

	case AST_NODE_TYPE_STRING:
	{
		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "string \"%s\"", node.data.string.value->raw);
	} break;

	case AST_NODE_TYPE_BOOLEAN:
	{
		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "boolean %s", node.data.boolean.value ? "true" : "false");
	} break;

	case AST_NODE_TYPE_IF:
	{
		dynstr* cond = ast_stringify_as_tree(*node.data.if_statement.cond, depth + 1);
		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "if:\n");
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "cond: \n    %s\n", cond->raw);

		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "then body:\n");

		for(u32 i = 0; i < node.data.if_statement.then_body_statements_len; i++)
		{
			dynstr* arg = ast_stringify_as_tree(node.data.if_statement.then_body_statements[i], depth + 2);
			dynstr_append(str, arg);
			dynstr_destroy(arg);
		}

		// @Todo: print body

		dynstr_destroy(cond);
	} break;

	case AST_NODE_TYPE_BINARY:
	{
		dynstr* left = ast_stringify_as_tree(*node.data.binary.left, depth + 2);
		dynstr* right = ast_stringify_as_tree(*node.data.binary.right, depth + 2);

		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "operation %s:\n", node.data.binary.op->raw);
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "left:\n%s\n", left->raw);
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "right:\n%s", right->raw);

		dynstr_destroy(left);
		dynstr_destroy(right);
	} break;

	case AST_NODE_TYPE_IDENTIFIER:
	{
		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "identifier %s", node.data.identifier.value->raw);
	} break;

	case AST_NODE_TYPE_MEMBER_ACCESS:
	{
		dynstr* owner = ast_stringify_as_tree(*node.data.member_access.owner, depth + 2);
		dynstr* member = ast_stringify_as_tree(*node.data.member_access.member, depth + 2);

		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "member access\n");
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "owner:\n%s\n", owner->raw);
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "member:\n%s", member->raw);
	} break;

	case AST_NODE_TYPE_FUNCTION_CALL:
	{
		dynstr* fid = ast_stringify_as_tree(*node.data.fcall.function_identifier, depth + 2);

		dynstr_append_char_repeat(str, ' ', 4 * depth);
		dynstr_append(str, "function call:\n");
		dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
		dynstr_append(str, "fid: \n%s", fid->raw);

		for(u32 i = 0; i < node.data.fcall.args_len; i++)
		{
			dynstr_append_char(str, '\n');
			dynstr_append_char_repeat(str, ' ', 4 * depth + 4);
			dynstr_append(str, "arg %i: \n", i);
			dynstr* arg = ast_stringify_as_tree(node.data.fcall.args[i], depth + 2);
			dynstr_append(str, arg);
			dynstr_destroy(arg);
		}

		dynstr_destroy(fid);
	} break;
	}

	return str;
}

dynstr* ast_stringify_as_tree(ast_node node)
{
	return ast_stringify_as_tree(node, 0);
}