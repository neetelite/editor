struct Position;

enum TokenKind
{
	token_null,

	token_identifier,
	token_punctuation,

	token_type,
	token_keyword,
	token_preprocessor,

	token_define,
	token_constant,

	token_function,
	token_function_declaration,

	token_variable,
	token_variable_declaration,

	token_number,

	token_i8,
	token_i16,
	token_i32,
	token_i64,

	token_f32,
	token_f64,

	token_comment_line,
	token_string, /* TODO: Remove */

	/* These can be multilined and are pushed to the buffer tokens */
	token_string_start,
	token_string_end,

	token_comment_open,
	token_comment_close,

	token_brackets_open,
	token_brackets_close,

	token_braces_open,
	token_braces_close,

	token_parenthesis_open,
	token_parenthesis_close,

	token_count,
};

struct Token
{
	enum TokenKind kind;
	f32 pos;
	u32 len;
};
