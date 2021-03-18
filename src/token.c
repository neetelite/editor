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
	token.kind = token_comment;
	token.pos = pos.x;

	position_update_next_char(&pos, &ptr);

	if(0) {}
	else if(*ptr.c == '/')
	{
		/* Line comment */
		token.len = ptr.line->char_count - token.pos;
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
read_token_identifier(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Position pos = *pos_in;
	struct PositionPointer ptr = *ptr_in;

	struct Token token = {0};
	token.kind = token_keyword;
	token.pos = pos.x;

	u32 len = 0;
	loop
	{
		struct Line *line = ptr.line;
		position_update_next_char(&pos, &ptr);
		len += 1;
		if(ptr.c == NULL || line->id != ptr.line->id ||
		   (!char_is_alphanumeric(*ptr.c) && *ptr.c != '_'))
		{
			break;
		}
	}

	token.len = len;
	line_token_push(ptr.line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
}

bool
read_token_punctuation(struct Position *pos_in, struct PositionPointer *ptr_in)
{
	struct Token token = {0};
	token.kind = token_type;
	token.pos = pos_in->x;
	token.len = 1;

	line_token_push(ptr_in->line, &token);
	position_update_next_char(pos_in, ptr_in);
}

void
line_tokenize(struct Panel *panel)
{
	/* NOTE: Finite state machine */
	struct Position pos = panel->pos;
	struct PositionPointer ptr = position_pointer_from_position(&pos);
	loop
	{
		if(ptr.c == NULL) break;

		switch(*ptr.c)
		{
		/* End of File */
		case '\0': return;

		/* Comment or Division */
		case '/':
		{
			if(read_token_comment(&pos, &ptr) == false)
			{
				/* TODO: */
				//token_punctuation_expected(panel, &at, '=');
			}
		} break;

		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
		{
			read_token_identifier(&pos, &ptr);
		} break;

		#if 0
		/* String */
		case '"':
		{
			read_token_string(panel, &at);
		} break;

		/* Number */
		case '0' ... '9':
		{
			read_token_number(panel, &at);
		} break;
		#endif

		/* Punctuation */
		case '#': case '@': case '$':
		case '^': case '~': case '.': case ',': case ';': case '=': case '\\':
		case '(': case ')':
		case '[': case ']':
		case '{': case '}':
		{
			read_token_punctuation(&pos, &ptr);
		} break;

		#if 0
		case '-': token_punctuation_expected(panel, &at, '='); break;
		case '+': token_punctuation_expected(panel, &at, '='); break;
		case '*': token_punctuation_expected(panel, &at, '='); break;

		case ':': token_punctuation_double(panel, &at); break;
		case '!': token_punctuation_double(panel, &at); break;
		case '&': token_punctuation_double(panel, &at); break;
		case '|': token_punctuation_double(panel, &at); break;
		case '%': token_punctuation_double(panel, &at); break;
		case '<': token_punctuation_double_or_expected(panel, &at, '?'); break;
		case '>': token_punctuation_double_or_expected(panel, &at, '?'); break;
		#endif

		default: position_update_next_char(&pos, &ptr); break; /* whitespace */
		}
	}
}
