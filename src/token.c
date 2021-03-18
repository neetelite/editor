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

	position_update_next_char(&pos, &ptr);

	if(0) {}
	else if(*ptr.c == '/')
	{
		pos.x = ptr.line->char_count;
		position_update_next_char(&pos, &ptr);
	}
	else if(*ptr.c == '*')
	{
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

	struct Token token = new_token_identifier(pos.x, len);
	token.meaning = token_keyword;

	line_token_push(ptr.line, &token);

	*pos_in = pos;
	*ptr_in = ptr;
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
		}

		#if 0
		/* Identifier */
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
		{
			token_identifier(panel, &at);
		} break;

		/* String */
		case '"':
		{
			token_string(panel, &at);
		} break;

		/* Number */
		case '0' ... '9':
		{
			token_number(panel, &at);
		} break;

		/* Punctuation */
		case '#': case '@': case '$':
		case '^': case '~': case '.': case ',': case ';': case '=': case '\\':
		case '(': case ')':
		case '[': case ']':
		case '{': case '}':
		{
			token_punctuation(panel, &at);
		} break;

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
