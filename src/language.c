void
language_keyword_init(struct Language *language, u32 max)
{
	ASSERT(language->keyword_max == 0);
	language->keyword_max = max;
	language->keywords = mem_alloc(max*sizeof(*language->keywords), false);
}

void
language_keyword_push(struct Language *language, CString cstr)
{
	String *keyword = &language->keywords[language->keyword_count];
	str_alloc(keyword, cstr);

	language->keyword_count += 1;
}

void
lang_c_keyword_init(void)
{
	struct Language *c = &languages[language_c];
	language_keyword_init(c, 32);
	language_keyword_push(c, "auto");
	language_keyword_push(c, "break");
	language_keyword_push(c, "case");
	language_keyword_push(c, "char");
	language_keyword_push(c, "contst");
	language_keyword_push(c, "continue");
	language_keyword_push(c, "default");
	language_keyword_push(c, "do");
	language_keyword_push(c, "double");
	language_keyword_push(c, "else");
	language_keyword_push(c, "enum");
	language_keyword_push(c, "extern");
	language_keyword_push(c, "float");
	language_keyword_push(c, "for");
	language_keyword_push(c, "goto");
	language_keyword_push(c, "if");
	language_keyword_push(c, "int");
	language_keyword_push(c, "long");
	language_keyword_push(c, "register");
	language_keyword_push(c, "return");
	language_keyword_push(c, "shoft");
	language_keyword_push(c, "signed");
	language_keyword_push(c, "sizeof");
	language_keyword_push(c, "static");
	language_keyword_push(c, "struct");
	language_keyword_push(c, "switch");
	language_keyword_push(c, "typedef");
	language_keyword_push(c, "union");
	language_keyword_push(c, "unsigned");
	language_keyword_push(c, "void");
	language_keyword_push(c, "volatile");
	language_keyword_push(c, "while");
}
