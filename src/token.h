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

	token_i8,
	token_i16,
	token_i32,
	token_i64,
	token_f32,
	token_f64,

	token_string,
	token_string_start,
	token_string_end,

	token_comment,
	token_comment_start,
	token_comment_end,

	token_brackets_start,
	token_brackets_end,

	token_braces_start,
	token_braces_end,

	token_parenthesis_start,
	token_parenthesis_end,

	token_count,
};

struct Token
{
	enum TokenKind kind;
	f32 pos;
	u32 len;
};
