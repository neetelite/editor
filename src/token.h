struct Position;

enum TokenMeaning
{
	token_meaning_null,

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

	token_meaning_count,
};

enum TokenKind
{
	token_null,

	token_identifier,
	token_punctuation,
	token_literal_number,
	token_literal_string,
};

struct Token
{
	u32 len;
	//struct Position pos;
	f32 pos;
	enum TokenKind kind;
	enum TokenMeaning meaning;
};

struct Token
new_token_identifier(f32 pos, u32 len)
{
	struct Token result = {0};
	result.len = len;
	//result.pos = *pos;
	result.pos = pos;
	result.kind = token_identifier;
	result.meaning = token_meaning_null;
	return(result);
}
