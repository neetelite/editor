void
line_token_push(struct Line *line, struct Token *token)
{
	if(line->token_max == 0)
	{
		line->token_max = 1;
		line->tokens = mem_alloc(line->token_max*sizeof(*line->tokens), true);
	}
	else if(line->token_count+1 > line->token_max)
	{
		line->token_max *= 2;
		line->tokens = mem_realloc(line->tokens,
					   line->token_max*sizeof(*line->tokens));
	}

	line->tokens[line->token_count] = *token;
	line->token_count += 1;
}

bool
read_token_comment(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.pos = pos.x;

	position_update_next_char(&pos, &ptr);

	if(0) {}
	else if(*ptr.c == '/')
	{
		/* Line comment */
		token.len = ptr.line->char_count - token.pos;
		token.kind = token_comment_line;
		line_token_push(ptr_in->line, &token);

		pos.x = ptr.line->char_count;
		position_update_next_char(&pos, &ptr);
	}
	else if(*ptr.c == '*')
	{
		/* TODO: Block comments can be multilined */
		/* Block comment */
		position_update_next_char(&pos, &ptr);

		u32 comment_count = 1;
		while(comment_count != 0)
		{
			if(*ptr.c == '/')
			{
				position_update_next_char(&pos, &ptr);
				if(*ptr.c == '*') comment_count += 1;
			}
			else if(*ptr.c == '*')
			{
				position_update_next_char(&pos, &ptr);
				if(*ptr.c == '/') comment_count -= 1;
			}
			position_update_next_char(&pos, &ptr);
		}

		token.len = pos.x - token.pos;
		token.kind = token_comment_open;
		line_token_push(ptr_in->line, &token);
	}
	else
	{
		return(false);
	}

	*pos_in = pos;
	*ptr_in = ptr;
	return(true);
}

bool
panel_eql_n(struct Position *position, CString cstr, u32 len)
{
	struct Position pos = *position;
	struct PositionPointer ptr = position_pointer_from_position(&pos);

	for(u32 i = 0; i < len; ++i)
	{
		if(*ptr.c != *cstr) return(false);
		position_update_next_char(&pos, &ptr);
		cstr += 1;
	}
	return(true);
}

bool
pos_is_keyword(struct Position *pos, u32 len)
{
	struct Buffer *buffer = editor_buffer_get_by_id(pos->b);
	struct Language *language = &languages[buffer->language];

	for(i32 i = 0; i < language->keyword_count; ++i)
	{
		String *keyword = &language->keywords[i];
		if(keyword->len != len) continue;
		if(panel_eql_n(pos, keyword->data, len)) return(true);
	}
	return(false);
}

bool
read_token_identifier(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.pos = pos.x;

	u32 len = 0;
	loop
	{
		struct Line *line = ptr.line;
		position_update_next_char(&pos, &ptr);
		len += 1;
		if(ptr.c == NULL || line->id != ptr.line->id ||
		   (!char_is_alnum(*ptr.c) && *ptr.c != '_'))
		{
			break;
		}
	}

	token.len = len;
	if(pos_is_keyword(pos_in, token.len)) token.kind = token_keyword;
	else token.kind = token_identifier;
	line_token_push(ptr.line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
read_token_string(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_string;
	token.pos = pos_in->x;

	loop
	{
		position_update_next_char(&pos, &ptr);
		if(ptr.c == NULL);
		else if(*ptr.c == '\\') position_update_next_char(&pos, &ptr);
		else if(*ptr.c == '"') break;
	}
	position_update_next_char(&pos, &ptr);

	token.len = pos.x - token.pos;
	line_token_push(ptr_in->line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
read_token_number(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_number;
	token.pos = pos_in->x;

	loop
	{
		position_update_next_char(&pos, &ptr);
		if(char_is_digit(*ptr.c) == false && *ptr.c != '.') break;
	}

	token.len = pos.x - token.pos;
	line_token_push(ptr_in->line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
read_token_punctuation(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Token token = {0};
	token.kind = token_punctuation;
	token.pos = pos_in->x;
	token.len = 1;

	line_token_push(ptr_in->line, &token);
	position_update_next_char(pos_in, ptr_in);
}

void
read_token_punctuation_expected(struct Position *pos_in, struct PositionPointer *ptr_in,
				char expected_char)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_punctuation;
	token.pos = pos_in->x;

	position_update_next_char(&pos, &ptr);
	if(*ptr.c == expected_char) token.len = 2;
	else token.len = 1;

	line_token_push(ptr_in->line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
read_token_punctuation_double(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_punctuation;
	token.pos = pos_in->x;

	char double_char = *ptr.c;
	position_update_next_char(&pos, &ptr);
	if(*ptr.c == double_char) token.len = 2;
	else token.len = 1;

	line_token_push(ptr_in->line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
read_token_punctuation_double_or_expected(struct Position *pos_in,
					  struct PositionPointer *ptr_in,
					  char expected_char)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_punctuation;
	token.pos = pos_in->x;

	char double_char = *ptr.c;
	position_update_next_char(&pos, &ptr);
	if(*ptr.c == expected_char || *ptr.c == double_char) token.len = 2;
	else token.len = 1;

	line_token_push(ptr_in->line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

void
line_tokenize(struct Position *position)
{
	struct Position pos = *position;
	struct PositionPointer ptr = position_pointer_from_position(&pos);

	ptr.line->token_count = 0;
	ptr.line->token_max = 0;
	if(ptr.line->tokens != NULL)
	{
		mem_free(ptr.line->tokens);
		ptr.line->tokens = NULL;
	}
	u32 line_init = pos.y;
	loop
	{
		if(ptr.line->id != line_init || ptr.c == NULL) break;

		switch(*ptr.c)
		{
		/* End of File */
		case '\0': return;

		/* Comment or Division */
		case '/':
		{
			if(read_token_comment(&pos, &ptr) == false)
			{
				read_token_punctuation_expected(&pos, &ptr, '=');
			}
		} break;

		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
		{
			read_token_identifier(&pos, &ptr);
		} break;

		/* String */
		case '"':
		{
			read_token_string(&pos, &ptr);
		} break;

		/* Number */
		case '0' ... '9':
		{
			read_token_number(&pos, &ptr);
		} break;

		/* Punctuation */
		case '#': case '@': case '$':
		case '^': case '~': case '.': case ',': case ';': case '=': case '\\':
		case '(': case ')':
		case '[': case ']':
		case '{': case '}':
		{
			read_token_punctuation(&pos, &ptr);
		} break;

		case '-': read_token_punctuation_expected(&pos, &ptr, '='); break;
		case '+': read_token_punctuation_expected(&pos, &ptr, '='); break;
		case '*': read_token_punctuation_expected(&pos, &ptr, '='); break;

		case ':': read_token_punctuation_double(&pos, &ptr); break;
		case '!': read_token_punctuation_double(&pos, &ptr); break;
		case '&': read_token_punctuation_double(&pos, &ptr); break;
		case '|': read_token_punctuation_double(&pos, &ptr); break;
		case '%': read_token_punctuation_double(&pos, &ptr); break;
		case '<': read_token_punctuation_double_or_expected(&pos, &ptr, '?'); break;
		case '>': read_token_punctuation_double_or_expected(&pos, &ptr, '?'); break;

		default: position_update_next_char(&pos, &ptr); break; /* whitespace */
		}
	}
}

void
buffer_tokenize(struct Panel *panel)
{
	struct Position pos = {0};
	pos.b = panel->pos.b;
	struct PositionPointer ptr = position_pointer_from_position(&pos);

	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	for(i32 i = 0; i < ptr.buffer->line_count; ++i)
	{
		struct Line *line = buffer_line_get_by_id(buffer, i);
		if(line->content_count == 0) continue;

		pos.y = line->id;
		pos.x = 0;
		pos.c = 0;
		pos.i = 0;
		line_tokenize(&pos);
	}
}
