void
line_content_shift_right(struct Line *line, u32 content_id, u32 n)
{
	struct Content tmp[n*2];
	for(i32 i = 0; i < n; ++i)
	{
		tmp[i] = line->contents[content_id+i];
	}

	/* TODO: Should it be i += n, or i += 2 */
	for(i32 i = content_id; i < line->content_count; i += n)
	{
		for(i32 j = 0; j < n; ++j)
		{
			tmp[j+n] = line->contents[i+j+n];
			line->contents[i+j+n] = tmp[j];
			tmp[j] = tmp[j+n];
		}
	}
}

void
content_data_shift_right(struct Content *content, u32 char_pos, u32 n)
{
	char tmp_0 = content->data[char_pos];
	char tmp_1 = '\0';

	for(i32 i = char_pos; i < content->char_count; ++i)
	{
		char *next = &content->data[i+1];
		char *current = &content->data[i];

		tmp_1 = *next;
		*next = tmp_0;
		tmp_0 = tmp_1;
	}
}

void
content_draw(struct Content *content)
{
	/* TEMPORARY: We need to find the exact char_width */
	f32 char_width = 10;
	f32 nw = editor->line_number_width;
	v2 pos = V2(editor->margin_left+nw+content->char_start*char_width, os_context.dim.height);
	cstr_draw(content->data, content->char_count, pos, &editor->align_buffer, V4_COLOR_WHITE);
}

	struct Line *line = &buffer->lines[buffer->position.y];
	struct Content *content = content_get_from_line_char_pos(line, buffer->position.x);

	u32 char_pos = content->char_start - buffer->position.x;
	struct Visual *visual = &content->visual[char_pos];

	v2 start = V2(nw+visual->rec.start.x, visual->rec.start.y);
	v2 end = V2(nw+visual->rec.end.x, visual->rec.end.y);


void
buffer_char_seek_next(struct Buffer *buffer, char c)
{
	u32 away = 0;

	struct Content *content = buffer_current_content(buffer);
	while(content->id != buffer->lines[buffer->position.y].content_count &&
	      buffer->position.x != content->char_start+content->char_count)
	{
		u32 char_pos = buffer->position.x - content->char_start;
		char x = content->data[char_pos];

		if(x == c)
		{
			printf("AWAY: %d\n", away);
			break;
		}

		++buffer->position.x;
		++away;

		if(buffer->position.x == content->char_start + content->char_count)
		{
			content = &buffer->lines[buffer->position.y].contents[content->id+1];
		}
	}
}

void
buffer_word_next(struct Buffer *buffer)
{
	struct Content *content = buffer_current_content(buffer);
	char prev = '\0';
	char at = '\0';
	//char next = '\0';

	v2i pos = V2I(buffer->position.x, buffer->position.y);

	/* TODO: This loop is not correct but it works */
	while(content->id != buffer->lines[pos.y].content_count &&
	      (u32)pos.x != content->char_start+content->char_count)
	{
		u32 char_pos = (u32)pos.x - content->char_start;
		at = content->data[char_pos];

		if(prev == ' ' && char_is_alph(at))
		{
			buffer->position.x = (u32)pos.x;
			buffer->position.y = (u32)pos.y;
			break;
		}
		else if(at == '_')
		{
			buffer->position.x = (u32)pos.x;
			buffer->position.y = (u32)pos.y;
			break;
		}

		if(content_char_index_is_end(content, (u32)pos.x)) content = buffer_content_get_next(buffer);
		prev = at;
		++pos.x;
		{
			content = &buffer->lines[pos.y].contents[content->id+1];
		}
	}
}
