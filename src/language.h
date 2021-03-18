enum LanguageType
{
	language_c,

	language_count,
};

struct Language
{
	u32 keyword_count;
	u32 keyword_max;
	String *keywords;
};
struct Language languages[language_count];
