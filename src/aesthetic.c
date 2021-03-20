void
aesthetics_init(void)
{
	for(i32 i = 0; i < token_count; ++i)
	{
		struct Aesthetic *aes = &aesthetics[i];
		switch(i)
		{
		case token_comment_line:
		case token_comment_open:
		case token_comment_close:
		{
			aes->foreground = vec4_mf(VEC4_COLOR_WHITE, 0.2);
		} break;
		case token_string:
		{
			aes->foreground = VEC4_COLOR_GREEN;
		} break;
		case token_number:
		{
			aes->foreground = VEC4_COLOR_CYAN;
		} break;
		case token_keyword:
		{
			aes->foreground = VEC4_COLOR_RED;
		} break;
		case token_type:
		{
			aes->foreground = VEC4_COLOR_BLUE;
		} break;
		}
	}
}

struct Aesthetic *
aesthetic_from_token(struct Token *token)
{
	if(token == NULL) return(NULL);

	struct Aesthetic *result = NULL;
	result = &aesthetics[token->kind];
	return(result);
}
