/* Content */
u32
content_char_index_from_pos(struct Content *content, u32 char_pos)
{
	u32 result = 0;
	if(content == NULL) result = 0;
	else result = char_pos - content->char_start;
	return(result);
}

u32
content_char_end(struct Content *content)
{
	u32 result = 0;
	if(content == NULL) result = 0;
	else result = content->char_start + content->char_count;
	return(result);
}

bool
content_char_index_is_end(struct Content *content, u32 char_index)
{
	bool result = false;
	if(char_index == content->size) result = true;
	return(result);
}

bool
content_char_pos_is_end(struct Content *content, u32 char_pos)
{
	bool result = false;
	if(char_pos == content->char_start+content->char_count) result = true;
	return(result);
}

bool
conten_char_index_is_last(struct Content *content, u32 char_index)
{
	bool result = false;
	if(content == NULL) result = false;
	else if(char_index == content->char_count-1) result = true;
	return(result);
}

bool
content_is_empty(struct Content *content)
{
	bool result = false;
	if(content->char_count == 0) result = true;
	return(result);
}

bool
content_is_full(struct Content *content)
{
	bool result = false;
	if(content->char_count == content->size) result = true;
	return(result);
}

/* Line */
struct Content *
line_content_get_by_id(struct Line *line, u32 content_id)
{
	ASSERT(content_id < line->content_count);

	struct Content *result = NULL;
	result = &line->contents[content_id];
	return(result);
}

struct Content *
line_content_get_by_char_pos(struct Line *line, u32 char_pos)
{
	struct Content *result = NULL;
	for(u32 i = 0; i < line->content_count; ++i)
	{
		struct Content *content = &line->contents[i];
		if(content->char_start <= char_pos && char_pos < content_char_end(content))
		{
			result = content;
			break;
		}
	}
	return(result);
}

struct Content *
line_content_get_first(struct Line *line)
{
	struct Content *result = NULL;
	result = line_content_get_by_id(line, 0);
	return(result);
}

struct Content *
line_content_get_last(struct Line *line)
{
	struct Content *result = NULL;
	result = line_content_get_by_id(line, line->content_count-1);
	return(result);
}

struct Content *
line_content_get_previous(struct Line *line, struct Content *content)
{
	struct Content *result = NULL;
	/* Empty */
	if(line_is_empty(line))
	{
		result = NULL;
	}
	/* Last */
	else if(content == NULL)
	{
		result = line_content_get_last(line);
	}
	/* First */
	else if(line_content_is_first(line, content))
	{
		result = NULL;
	}
	/* Middle */
	else
	{
		result = line_content_get_by_id(line, content->id-1);
	}
	return(result);
}

struct Content *
line_content_get_next(struct Line *line, struct Content *content)
{
	struct Content *result = NULL;

	/* Empty */
	if(line_is_empty(line))
	{
		result = NULL;
	}
	/* Last */
	else if(content == NULL)
	{
		result = NULL;
	}
	/* First */
	else if(line_content_is_last(line, content))
	{
		result = NULL;
	}
	/* Middle */
	else
	{
		result = line_content_get_by_id(line, content->id+1);
	}

	return(result);
}

bool
line_content_is_first(struct Line *line, struct Content *content)
{
	bool result = false;
	if(content->id == 0) result = true;
	return(result);
}

bool
line_content_is_last(struct Line *line, struct Content *content)
{
	bool result = false;
	if(content->id == line->content_count-1) result = true;
	return(result);
}

bool
line_is_empty(struct Line *line)
{
	bool result = false;
	if(line->content_count == 0) result = true;
	else
	{
		struct Content *content = line_content_get_first(line);
		if(content_is_empty(content)) result = true;
	}
	return(result);
}

bool
line_is_full(struct Line *line)
{
	bool result = false;
	if(line->content_count == 0) result = false;
	else
	{
		struct Content *content = line_content_get_last(line);
		if(content_is_full(content)) result = true;
	}
	return(result);
}

/* Buffer */
#if 0
struct Content *
position_content_get_previous(struct Panel *panel)
{
	struct Content *result = NULL;
	if(panel->pos.c == EOL)
	{
		result = line_content_get_last(pointer.line);
	}
	else if(line_content_is_first(pointer.line, pointer.content))
	{
		struct Line *line = panel_line_get_previous(panel);
		result = line_content_get_last(line);
	}
	else
	{
		result = line_content_get_by_id(pointer.line, pointer.content->id-1);
	}
	return(result);
}

struct Content *
position_content_get_next(struct Panel *panel)
{
	struct Content *result = NULL;
	if(pointer.content == NULL)
	{
		/* TODO: */
	}
	else if(pointer.content->id == pointer.line->content_count-1)
	{
		struct Line *line = panel_line_get_next(panel);
		result = line_content_get_first(line);
	}
	else
	{
		result = line_content_get_by_id(pointer.line, pointer.content->id+1);
	}
	return(result);
}

struct Line *
position_line_get_previous(struct Panel *panel)
{
	struct Line *result = NULL;
	/* TEMPORARY TODO */
	result = buffer_line_get_by_id(panel->buffer, panel->pos.y-1);
	return(result);
}

struct Line *
position_line_get_next(struct Panel *panel)
{
	struct Line *result = NULL;
	/* TEMPORARY TODO */
	result = buffer_line_get_by_id(panel->buffer, panel->pos.y+1);
	return(result);
}
#endif

/* Buffer */
struct Line *
buffer_line_get_by_id(struct Buffer *buffer, u32 line_id)
{
	ASSERT(line_id < buffer->line_count);

	struct Line *result = NULL;
	result = &buffer->lines[line_id];
	return(result);
}

/* Editor */
struct Buffer *
editor_buffer_get_by_id(u32 buffer_id)
{
	ASSERT(buffer_id < editor->buffer_count);

	struct Buffer *result = NULL;
	result = &editor->buffers[buffer_id];
	return(result);
}

struct Panel *
window_panel_get_by_id(struct Window *window, u32 panel_id)
{
	ASSERT(panel_id < window->panel_count);

	struct Panel *result = NULL;
	result = &window->panels[panel_id];
	return(result);
}
