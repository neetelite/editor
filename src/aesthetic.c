void
aesthetics_init(void)
{
	struct Aesthetic *keyword = &aesthetics[token_keyword];
	keyword->foreground = V4_COLOR_RED;

	struct Aesthetic *type = &aesthetics[token_type];
	type->foreground = V4_COLOR_BLUE;
}

struct Aesthetic *
aesthetic_from_token(struct Token *token)
{
	if(token == NULL) return(NULL);

	struct Aesthetic *result = NULL;
	result = &aesthetics[token->meaning];
	return(result);
}
