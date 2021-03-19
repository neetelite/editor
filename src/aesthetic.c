void
aesthetics_init(void)
{
	for(i32 i = 0; i < token_count; ++i)
	{
		struct Aesthetic *aes = &aesthetics[i];
		switch(i)
		{
		case token_comment:
		{
			aes->foreground = v4_mf(V4_COLOR_WHITE, 0.2);
		} break;
		case token_string:
		{
			aes->foreground = V4_COLOR_GREEN;
		} break;
		case token_number:
		{
			aes->foreground = V4_COLOR_YELLOW;
		} break;
		case token_keyword:
		{
			aes->foreground = V4_COLOR_RED;
		} break;
		case token_type:
		{
			aes->foreground = V4_COLOR_BLUE;
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
