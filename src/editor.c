void
cstr_draw(char *text, u32 len, v2 pos, struct Alignment *align, f32 z, v4 color)
{
	//cstr_draw_full(&editor->font, text, len, pos, align, gl->projection_2d, 0.0, color);
	cstr_draw_full(&editor->font, text, len, pos, align, editor->camera.transform, z, color);
}

struct Position
POS(u32 b, u32 y, u32 x, i32 c, i32 i)
{
	struct Position result = {0};
	result.b = b;
	result.y = y;
	result.x = x;
	result.c = c;
	result.i = i;
	return(result);
}

struct PositionPointer
position_pointer_from_position(struct Position *position)
{
	struct PositionPointer result = {0};
	result.buffer = editor_buffer_get_by_id(position->b);
	result.line = buffer_line_get_by_id(result.buffer, position->y);

	if(position->c == -1)
	{
		result.content = EOL_PTR;
		result.c = EOL_PTR;
	}
	else
	{
		result.content = line_content_get_by_id(result.line, position->c);
		result.c = &result.content->data[position->i];
	}
	return(result);
}

/* Content */
struct Content
content_new(u32 id, u32 start, u32 count)
{
	/* TODO: Expand this to be able to add content in bettwen other content by passing the line pointer */

	struct Content result = {0};
	result.id = id;
	result.char_start = start;
	result.char_count = count;
	result.size = editor->content_min;
	result.size_alloc = editor->content_min;

	u64 size_data = result.size_alloc * sizeof(*result.data);
	u64 size_visual = result.size_alloc * sizeof(*result.visual);
	u64 size_total =  size_data + size_visual;
	result.data = mem_alloc(size_total, false);
	result.visual = (struct Visual *)((u08 *)result.data+size_data);

	return(result);
}

void
content_data_shift_update(struct Position *position)
{
	struct PositionPointer pointer = position_pointer_from_position(position);

	struct Content *current = pointer.content;
	struct Content *previous = line_content_get_previous(pointer.line, current);

	f32 scale = 1.0;

	v2 pos = V2_ZERO;
	u32 codepoint_previous;
	if(position->i == 0)
	{
		if(position->c == 0)
		{
			codepoint_previous = 0;
			pos.x = 0.0;
		}
		else
		{
			u32 previous_index = previous->char_count-1;
			codepoint_previous = previous->data[previous_index];
			pos.x = previous->visual[previous_index].rec.start.x;
		}
	}
	else
	{
		u32 previous_index = position->i - 1;
		codepoint_previous = current->data[previous_index];
		pos.x = current->visual[previous_index].rec.start.x;
	}
	pos.y = HEIGHT - (editor->font.height * (1 + position->y));

	v2 min, max;
	for(u32 i = position->i; i < current->char_count; ++i)
	{
		struct Visual *visual = &current->visual[i];
		u32 codepoint = current->data[i];

		v2 advance = V2_ZERO;
		advance.x = scale*font_kerning_get(&editor->font, codepoint, codepoint_previous);
		pos.x += advance.x;

		struct GL_Texture texture = font_glyph_texture_get(&editor->font, codepoint);

		v2 baseline_align = font_glyph_alignment_get(&editor->font, codepoint);
		baseline_align = v2_m(baseline_align, V2(texture.image.width, texture.image.height));

		v2 render_pos = v2_s(pos, baseline_align);

		if(char_is_whitespace(codepoint))
		{
			f32 end_x = font_kerning_get(&editor->font, codepoint, codepoint);

			min = render_pos;
			max = v2_a(render_pos, V2(end_x, editor->font.height));
		}
		else
		{
			min = render_pos;
			max = v2_a(render_pos, V2(texture.image.width, texture.image.height));
		}
		visual->rec = REC2(min, max);

		codepoint_previous = codepoint;
	}
}

void
content_data_shift_right(struct Position *pos, u32 n)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	for(u32 i = pointer.content->char_count+n-1; i > pos->x; --i)
	{
		pointer.content->data[i] = pointer.content->data[i-n];
	}
}

void
content_char_draw(struct Content *content, u32 char_index, v4 color)
{
	f32 nw = editor->line_number_width;
	mat4 mat_view = editor->camera.transform;

	struct GL_ProgramText *program = &gl->program_text;
	gl_program_bind(program->handle);

	gl_uniform_v4(program->location_color, color);

	struct Visual *visual = &content->visual[char_index];
	u32 codepoint = content->data[char_index];

	struct GL_Texture texture = font_glyph_texture_get(&editor->font, codepoint);
	gl_texture_bind(&texture, GL_TEXTURE_2D);

	v2 min = v2_a(visual->rec.start, V2(editor->margin_left+nw, 0));
	v2 max = v2_a(visual->rec.end, V2(editor->margin_left+nw, 0));

	struct Rec2 rec = REC2(min, max);
	struct Box2 box = box2_from_rec2(rec);

	v3 pos = V3(box.pos.x, box.pos.y, canvas->z[layer_text]);
	v3 dim = V3(box.dim.x, box.dim.y, 0.0);
	mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, V3_ZERO));
	gl_uniform_mat4(program->location_mvp, mvp);

	u32 triangle_count = 2;
	gl_vao_draw(0, triangle_count * 3, GL_TRIANGLES);

	gl_texture_unbind(GL_TEXTURE_2D);
	gl_program_unbind();
}

void
panel_content_draw(struct Panel *panel, struct Content *content)
{
	/* TEMPORARY: We need to find the exact char_width */
	f32 nw = editor->line_number_width;
	mat4 mat_view = editor->camera.transform;
	v4 color = V4_COLOR_WHITE;

	struct GL_ProgramText *program = &gl->program_text;
	gl_program_bind(program->handle);

	gl_uniform_v4(program->location_color, color);

	for(u32 i = 0; i < content->char_count; ++i)
	{
		struct Visual *visual = &content->visual[i];
		u32 codepoint = content->data[i];

		struct GL_Texture texture = font_glyph_texture_get(&editor->font, codepoint);
		gl_texture_bind(&texture, GL_TEXTURE_2D);

		v2 min = v2_a(visual->rec.start, V2(editor->margin_left+nw, 0));
		v2 max = v2_a(visual->rec.end, V2(editor->margin_left+nw, 0));

		struct Rec2 rec = REC2(min, max);
		struct Box2 box = box2_from_rec2(rec);

		v3 pos = V3(box.pos.x, box.pos.y, canvas->z[layer_text]);
		v3 dim = V3(box.dim.x, box.dim.y, 0.0);
		mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, V3_ZERO));
		gl_uniform_mat4(program->location_mvp, mvp);

		u32 triangle_count = 2;
		gl_vao_draw(0, triangle_count * 3, GL_TRIANGLES);
	}

	gl_texture_unbind(GL_TEXTURE_2D);
	gl_program_unbind();
}

/* Line */
struct Line
line_new(u32 id)
{
	struct Line result = {0};
	result.id = id;
	result.content_count = 1;
	result.content_max = 2;
	result.contents = mem_alloc(result.content_max * sizeof(*result.contents), false);
	result.contents[0] = content_new(0, 0, 0);
	return(result);
}

void
line_grow(struct Line *line)
{
	struct Content *old_contents = line->contents;

	u32 old_max = line->content_max;
	u32 new_max = old_max * 2;

	u32 old_size = old_max * sizeof(*line->contents);
	u32 new_size = old_size * 2;

	struct Content *new_contents = mem_alloc(new_size, true);
	mem_cpy(new_contents, old_contents, old_size);
	mem_free(old_contents);

	line->content_max = new_max;
	line->contents = new_contents;
}

void
buffer_grow(struct Buffer *buffer)
{
	struct Line *old_lines = buffer->lines;

	u32 old_max = buffer->line_max;
	u32 new_max = old_max * 2;

	u32 old_size = old_max * sizeof(*buffer->lines);
	u32 new_size = old_size * 2;

	struct Line *new_lines = mem_alloc(new_size, true);
	mem_cpy(new_lines, old_lines, old_size);
	mem_free(old_lines);

	buffer->line_max = new_max;
	buffer->lines = new_lines;
}

void
line_content_shift_update(struct Position *pos)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);

	struct Position position = *pos;
	for(u32 content_id = pos->c; content_id < pointer.line->content_count; ++content_id)
	{

		struct Content *current = line_content_get_by_id(pointer.line, content_id);
		current->id = content_id;
		if(content_id == 0)
		{
			current->char_start = 0;
		}
		else
		{
			struct Content *previous = line_content_get_by_id(pointer.line, content_id - 1);
			current->char_start = previous->char_start + previous->char_count;
		}

		position.c = content_id;
		if(content_id != pointer.content->id) position.i = 0;

		content_data_shift_update(&position);
	}

	struct Content *last = line_content_get_last(pointer.line);
	pointer.line->char_count = last->char_start + last->char_count;
}

void
line_content_shift_right(struct Position *pos, u32 n)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);

	if(pointer.line->content_count+n > pointer.line->content_max)
	{
		line_grow(pointer.line);
	}

	for(u32 content_id = pointer.line->content_count+n-1; content_id > pos->c; --content_id)
	{
		pointer.line->contents[content_id] = pointer.line->contents[content_id-n];
	}
}

void
line_content_add_before(struct Position *pos)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	if(pointer.line->content_count+1 > pointer.line->content_max) line_grow(pointer.line);

	line_content_shift_right(pos, 1);

	pointer = position_pointer_from_position(pos);
	*pointer.content = content_new(pos->c, pointer.content->char_start, 0);

	pointer.line->content_count += 1;
}

void
line_content_add_end(struct Position *pos)
{
	/* NOTE: Only use for the last content in the line */
	struct PositionPointer pointer = position_pointer_from_position(pos);
	if(pointer.line->content_count+1 > pointer.line->content_max) line_grow(pointer.line);

	struct Content *last = line_content_get_last(pointer.line);
	struct Content *end = &pointer.line->contents[pointer.line->content_count];
	*end = content_new(last->id+1, content_char_end(last), 0);

	pointer.line->content_count += 1;

	pos->c = end->id;
	pos->i = 0;
}

void
line_content_add_between(struct Position *pos)
{
	/* TODO: Change this to char_index */
	struct PositionPointer pointer = position_pointer_from_position(pos);
	ASSERT(content_is_full(pointer.content) == true);

	if(pointer.line->content_count+2 > pointer.line->content_max)
	{
		line_grow(pointer.line);
		pointer = position_pointer_from_position(pos);
	}

	if(pos->c < pointer.line->content_count-1)
	{
		struct Position position = *pos;
		position.c += 1;

		line_content_shift_right(&position, 2);
		pointer = position_pointer_from_position(pos);
	}

	struct Content *before = pointer.content;
	before->id = pos->c;
	before->char_start = pointer.content->char_start;
	before->char_count = pos->i;
	before->size = before->char_count;
	before->size_alloc = pointer.content->size_alloc;
	before->data = pointer.content->data;
	before->visual = pointer.content->visual;

	struct Content *between = pointer.content+1;
	*between = content_new(pos->c + 1, content_char_end(before), 0);

	struct Content *after = pointer.content+2;
	after->id = before->id + 2;
	after->char_start = between->char_start;
	after->char_count = before->size_alloc - before->char_count;
	after->size = after->char_count;
	after->size_alloc = before->size_alloc;
	after->data = before->data + before->size;
	after->visual = before->visual + before->size;

	pointer.line->content_count += 2;

	pos->c += 1;
	pos->i = 0;
}

void
line_content_input(struct Position *pos, char c)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	ASSERT(pos->i <= pointer.content->char_count);

	if(pos->i < pointer.content->char_count)
	{
		content_data_shift_right(pos, 1);
	}

	pointer.content->data[pos->i] = c;
	pointer.content->char_count += 1;

	line_content_shift_update(pos);
}

void
panel_line_number_draw(struct Panel *panel, struct Line *line)
{
	f32 char_height = editor->font.height;
	v2 pos = V2(editor->margin_left, HEIGHT-(line->id*char_height));
	char content[5];

	snprintf(content, 5, "%d", line->id+1);
	cstr_draw(content, (line->id/10)+1, pos, &editor->align_buffer, canvas->z[layer_text], V4_COLOR_BLUE);
}

void
panel_line_draw(struct Panel *panel, struct Line *line)
{
	panel_line_number_draw(panel, line);
	for(u32 i = 0; i < line->content_count; ++i)
	{
		struct Content *content = &line->contents[i];
		panel_content_draw(panel, content);
	}
}

/* Buffer */
struct Buffer
buffer_new(void)
{
	struct Buffer result = {0};
	result.line_count = 1;
	result.line_max = 1;
	result.lines = mem_alloc(result.line_max * sizeof(*result.lines), false);
	result.lines[0] = line_new(0);
	return(result);
}

#if 0
void
buffer_shift_down(struct Buffer *buffer, u32 line_id, u32 n)
{
	for(i32 i = line_id; i < buffer->line_count; i += n)
	{

	}
}

void
buffer_line_add_above(struct Buffer *buffer)
{
	/* TODO: Test of shifting */

	if(buffer->line_count+1 > buffer->line_max) buffer_grow(buffer);

	u32 line_id = panel->pos.y;
	struct Line line = line_new(line_id);

	buffer->line_count += 1;
	printf("line added\n");
}
#endif

void
panel_cursor_move_up(struct Panel *panel)
{
	if(panel->pos.y <= 0) return;

	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.y -= 1;
	pointer.line = buffer_line_get_by_id(pointer.buffer, panel->pos.y);

	if(panel->pos.x_max_active)
	{
		if(panel->pos.x_max > pointer.line->char_count)
		{
			panel->pos.c = EOL;
			panel->pos.i = EOL;

			struct Content *last = line_content_get_last(pointer.line);
			panel->pos.x = content_char_end(last);
		}
		else
		{
			panel->pos.x = panel->pos.x_max;

			struct Content *content = line_content_get_by_char_pos(pointer.line, panel->pos.x);
			if(content == NULL)
			{
				panel->pos.c = EOL;
				panel->pos.i = EOL;
			}
			else
			{
				panel->pos.c = content->id;
				panel->pos.i = content_char_index_from_pos(content, panel->pos.x);
			}
		}
	}
	else
	{
		if(panel->pos.x_max > pointer.line->char_count)
		{
			panel->pos.c = EOL;
			panel->pos.i = EOL;

			panel->pos.x_max = panel->pos.x;
			panel->pos.x_max_active = true;

			struct Content *last = line_content_get_last(pointer.line);
			panel->pos.x = content_char_end(last);
		}
		else
		{
			struct Content *content = line_content_get_by_char_pos(pointer.line, panel->pos.x);
			panel->pos.c = content->id;
			panel->pos.i = content_char_index_from_pos(content, panel->pos.x);
		}
	}
}

void
panel_cursor_move_down(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	if(panel->pos.y >= pointer.buffer->line_count-1) return;

	panel->pos.y += 1;
	pointer.line = buffer_line_get_by_id(pointer.buffer, panel->pos.y);
	//pointer.content = line_content_get_by_char_pos(pointer.line, panel->pos.x);

	if(panel->pos.x > pointer.line->char_count)
	{
		panel->pos.c = EOL;
		panel->pos.i = EOL;

		if(panel->pos.x_max_active == false)
		{
			panel->pos.x_max = panel->pos.x;
			panel->pos.x_max_active = true;
		}

		struct Content *last = line_content_get_last(pointer.line);
		panel->pos.x = content_char_end(last);
	}
	else
	{
		if(panel->pos.x_max_active == true)
		{
			panel->pos.x = panel->pos.x_max;
		}
		else
		{
			panel->pos.c = pointer.content->id;
			panel->pos.i = content_char_index_from_pos(pointer.content, panel->pos.x);
		}
	}
}

void
panel_cursor_move_right(struct Panel *panel)
{
	panel->pos.x_max_active = false;

	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	if(panel->pos.x > pointer.line->char_count-1) return;

	panel->pos.x += 1;
	if(panel->pos.x >= content_char_end(pointer.content))
	{
		pointer.content = line_content_get_next(pointer.line, pointer.content);
		if(pointer.content != EOL_PTR)
		{
			panel->pos.c = pointer.content->id;
			panel->pos.i = 0;
		}
		else
		{
			panel->pos.c = EOL;
			panel->pos.i = EOL;
		}
	}
	else
	{
		panel->pos.i += 1;
	}
}

void
panel_cursor_move_left(struct Panel *panel)
{
	panel->pos.x_max_active = false;

	if(panel->pos.x <= 0) return;
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.x -= 1;
	if(pointer.content == EOL_PTR)
	{
		pointer.content = line_content_get_last(pointer.line);
		panel->pos.c = pointer.content->id;
		panel->pos.i = pointer.content->char_count-1;
	}
	else if(panel->pos.x < pointer.content->char_start)
	{
		pointer.content = line_content_get_previous(pointer.line, pointer.content);
		panel->pos.i = pointer.content->char_count-1;
		panel->pos.c = pointer.content->id;
	}
	else
	{
		panel->pos.i -= 1;
	}
}

void
panel_cursor_move_start(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	pointer.content = line_content_get_first(pointer.line);
	if(pointer.content)
	{
		panel->pos.c = 0;
		panel->pos.i = 0;
	}
	else
	{
		panel->pos.c = EOL;
		panel->pos.i = EOL;
	}
	panel->pos.x = 0;
}

void
panel_cursor_move_end(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.c = EOL;
	panel->pos.i = EOL;
	panel->pos.x = pointer.line->char_count;
}

/* TODO: Change this to panel */
void
panel_cursor_draw(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	/* TEMPORARY: We need to find the exact char_width */
	f32 nw = editor->line_number_width;
	nw += editor->margin_left;
	f32 char_height = editor->font.height;

	v2 cursor_dim = V2(10, char_height);
	v2 cursor_pos = V2(0, HEIGHT-((panel->pos.y+1)*char_height));

	struct Line *line = &pointer.buffer->lines[panel->pos.y];
	struct Content *content = line_content_get_by_char_pos(line, panel->pos.x);

	v2 start, end;
	if(content == NULL)
	{
		#if 0
		/* End of the line */
		u32 char_index = panel->pos.x - content->char_start;

		struct Visual *previous = &content->visual[char_index - 1];
		start = V2(nw+previous->rec.end.x, cursor_pos.y);
		end = V2(start.x + cursor_dim.x, cursor_pos.y + cursor_dim.y);
		#endif

		content = line_content_get_last(line);
	}

	if(panel->pos.x == 0)
	{
		/* TODO: Handle newline, space and null chars and remove this if statement */
		start = V2(nw+cursor_pos.x, cursor_pos.y);
		end = V2(start.x+cursor_dim.x, start.y+cursor_dim.y);
	}
	else
	{
		u32 char_index = panel->pos.x - content->char_start;
		if(char_index == 0)
		{
			struct Content *previous = line_content_get_previous(pointer.line, pointer.content);
			struct Visual *visual = &previous->visual[previous->char_count-1];

			start = V2(nw+visual->rec.end.x, cursor_pos.y);
			end = V2(start.x + cursor_dim.x, cursor_pos.y + cursor_dim.y);
		}
		else
		{
			struct Visual *previous = &content->visual[char_index - 1];
			//struct Visual *current = &content->visual[char_index];

			start = V2(nw+previous->rec.end.x, cursor_pos.y);
			#if 0
			end = V2(nw+current->rec.start.x, cursor_pos.y + cursor_dim.y);
			#else
			end = V2(start.x + cursor_dim.x, cursor_pos.y + cursor_dim.y);
			#endif
		}
	}

	struct Rec2 cursor = REC2(start, end);
	rec2_draw(&cursor, editor->camera.transform, canvas->z[layer_cursor], V4_COLOR_RED);

	/* TODO: Text */
	//content_char_draw(pointer.content, panel->pos.i, editor->color_background);
}

void
panel_line_input(struct Panel *panel, char c)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	/* End of the line */
	if(panel->pos.c == EOL)
	{
		if(line_is_empty(pointer.line))
		{
			/* TEMPORARY TODO */
			//*pointer.content = content_new(0, 0, 0);
			panel->pos.c = 0;
			panel->pos.i = 0;
		}
		else
		{
			line_content_add_end(&panel->pos);
		}
	}
	/* Full */
	else if(content_is_full(pointer.content))
	{
		u32 char_index = content_char_index_from_pos(pointer.content, panel->pos.x);
		if(char_index == 0)
		{
			line_content_add_before(&panel->pos);
		}
		else
		{
			/* TODO: Change this to char_index */
			line_content_add_between(&panel->pos);
		}
	}

	line_content_input(&panel->pos, c);
	panel_cursor_move_right(panel);
}

void
panel_buffer_draw(struct Panel *panel)
{
	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);

	for(u32 i = 0; i < buffer->line_count; ++i)
	{
		struct Line *line = &buffer->lines[i];
		panel_line_draw(panel, line);
	}
}

/* Panel */
struct Panel
panel_new(struct Buffer *buffer)
{
	struct Panel result = {0};
	result.pos.b = 0;
	result.pos.c = 0;
	result.pos.i = 0;
	result.pos.x = 0;
	result.pos.y = 0;
	return(result);
}

void
panel_bar_draw(struct Panel *panel)
{
	/* Edit Mode */
	cstr_draw(edit_mode_str_table[panel->edit_mode], EDIT_MODE_STR_SIZE,
		  V2(0, 0), &editor->align_bar, canvas->z[layer_text], V4_COLOR_WHITE);
}

void
panel_draw(struct Panel *panel)
{
	panel_bar_draw(panel);
	panel_buffer_draw(panel);
	panel_cursor_draw(panel);
}

/* Editor */
void
panel_edit_mode_change_insert(struct Panel *panel)
{
	switch(panel->edit_mode)
	{
	case edit_mode_insert: break;
	case edit_mode_normal: break;
	}

	panel->edit_mode = edit_mode_insert;
}

void
panel_edit_mode_change_normal(struct Panel *panel)
{
	switch(panel->edit_mode)
	{
	case edit_mode_insert:
	{
		//panel_cursor_move_left(panel);
	} break;
	case edit_mode_normal: break;
	}

	panel->edit_mode = edit_mode_normal;
}

void
panel_edit_mode_change(struct Panel *panel, enum EditMode edit_mode)
{
	switch(edit_mode)
	{
	case edit_mode_insert: panel_edit_mode_change_insert(panel); break;
	case edit_mode_normal: panel_edit_mode_change_normal(panel); break;
	}
}

struct Window
window_new()
{
	struct Window result = {0};
	result.panel_id = 0;
	result.panel_count = editor->buffer_count; /* TEMPORARY TODO:  */
	result.panels = mem_alloc(result.panel_count * sizeof(*result.panels), true);
	for(u32 i = 0; i < result.panel_count; ++i)
	{
		result.panels[i] = panel_new(&editor->buffers[i]);
	}
	return(result);
}

void
editor_init(void)
{
	editor = &app->editor;

	/* Settings */
	font_init(&editor->font, "ibm.ttf", "IBM Plex Mono", 20, "ascii");
	//font_init(&editor->font, "arial.ttf", "Arial", 20, "ascii");

	editor->align_bar    = ALIGN("left", "bottom");
	editor->align_buffer = ALIGN("left", "top");
	editor->line_number_width = 20.0;
	editor->margin_left = 5.0;
	editor->color_background = v4_mf(V4_COLOR_WHITE, 0.05);
	gl_viewport_color_set(editor->color_background);
	#if 1
	editor->content_min = 256;
	editor->content_max = 256;
	#else
	editor->content_min = 2;
	editor->content_max = 2;
	#endif

	/* Camera */
	camera_default_load(&editor->camera, 0, "main");
	editor->camera.pos = V3(WIDTH/2, HEIGHT/2, 1);
	editor->camera.projection_type = camera_projection_orthographic;
	editor->camera.zoom = 1.0;
	camera_rot_set(&editor->camera, V3(-90, 0, 180));

	/* Data */
	/* Buffer */
	editor->buffer_count = 1;
	editor->buffers = mem_alloc(editor->buffer_count * sizeof(*editor->buffers), true);
	editor->buffers[0] = buffer_new();

	/* Window */
	editor->window_id = 0;
	editor->window_count = 1;
	editor->windows = mem_alloc(editor->window_count * sizeof(*editor->windows), true);
	editor->windows[0] = window_new(&editor->buffers[0]);
}

void
position_update_next_line(struct Position *position,
			  struct PositionPointer *pointer)
{
	position->y += 1;
}

void
position_update_previous_char(struct Position *position,
			      struct PositionPointer *pointer)
{
	if(pointer->c == NULL)
	{
		if(line_is_empty(pointer->line))
		{
			return;
		}
		else
		{
			position->x -= 1;
			position->c = pointer->line->content_count-1;
			pointer->content = line_content_get_by_id(pointer->line, position->c);

			position->i = pointer->content->char_count-1;
			pointer->c = &pointer->content->data[position->i];
		}
	}
	else if(position->x == 0)
	{
		if(position->y == 0)
		{
			return;
		}
		else
		{
			position->y -= 1;
			pointer->line = buffer_line_get_by_id(pointer->buffer, position->y);
			position->x = pointer->line->char_count-1;

			position->c = pointer->line->content_count-1;
			pointer->content = line_content_get_by_id(pointer->line, position->c);

			position->i = pointer->content->char_count-1;
			pointer->c = &pointer->content->data[position->i];
		}
	}
	else
	{
		position->x -= 1;
		if(position->i == 0)
		{
			position->c -= 1;
			pointer->content = line_content_get_by_id(pointer->line, position->c);

			position->i = pointer->content->char_count-1;
			pointer->c = &pointer->content->data[position->i];
		}
		else
		{
			position->i -= 1;
			pointer->c = &pointer->content->data[position->i];
		}
	}
}

void
position_update_next_char(struct Position *position,
			  struct PositionPointer *pointer)
{
	if(position->c == EOL) return;

	position->x += 1;
	/* End of Line */
	if(position->x >= pointer->line->char_count)
	{
		/* End of buffer */
		if(pointer->line->id >= pointer->buffer->line_count-1)
		{
			position->c = EOL;
			position->i = EOL;
			pointer->content = EOL_PTR;
			pointer->c = EOL_PTR;

			return;
		}
		/* New line */
		else
		{
			position->y += 1;
			position->c = 0;

			pointer->line = buffer_line_get_by_id(pointer->buffer, position->y);
			pointer->content = line_content_get_by_id(pointer->line, position->c);
			pointer->c = &pointer->content->data[position->i];
		}
	}
	/* End of content */
	else if(position->x >= content_char_end(pointer->content))
	{
		position->c += 1;
		pointer->content = line_content_get_by_id(pointer->line, position->c);

		position->i = 0;
		pointer->c = &pointer->content->data[position->i];
	}
	else
	{
		position->i += 1;
		pointer->c = &pointer->content->data[position->i];
	}
}

void
panel_cursor_move_word_previous(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	position_update_previous_char(&panel->pos, &pointer);
	loop
	{
		for(u32 i = 0; i < COUNT(word_tokens_stop); ++i)
		{
			if((u32)*pointer.c == word_tokens_stop[i])
			{
				return;
			}
		}

		position_update_previous_char(&panel->pos, &pointer);

		if(panel->pos.y == 0 && panel->pos.x == 0) return;

		for(u32 i = 0; i < COUNT(word_tokens_skip); ++i)
		{
			if((u32)*pointer.c == word_tokens_skip[i])
			{
				position_update_next_char(&panel->pos, &pointer);
				return;
			}
		}
	}
}

void
panel_cursor_move_word_next(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	char *previous = pointer.c;
	u32 previous_y = panel->pos.y;
	loop
	{
		position_update_next_char(&panel->pos, &pointer);

		if(pointer.c == NULL) return;
		if(previous_y < panel->pos.y) return;

		for(u32 i = 0; i < COUNT(word_tokens_skip); ++i)
		{
			if((u32)*previous == word_tokens_skip[i])
			{
				return;
			}
		}

		for(u32 i = 0; i < COUNT(word_tokens_stop); ++i)
		{
			if((u32)*pointer.c == word_tokens_stop[i])
			{
				return;
			}
		}

		previous = pointer.c;
	}
}

void
panel_line_add_bellow(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	if(pointer.buffer->line_count+1 >= pointer.buffer->line_max)
	{
		buffer_grow(pointer.buffer);
		pointer = position_pointer_from_position(&panel->pos);
	}

	u32 line_id = panel->pos.y+1;
	pointer.buffer->lines[line_id] = line_new(line_id);
	pointer.buffer->line_count += 1;

	panel_cursor_move_down(panel);
	panel_edit_mode_change(panel, edit_mode_insert);
}

void
panel_input(struct Panel *panel)
{
	switch(panel->edit_mode)
	{
	case edit_mode_normal:
	{
		/* Modes */
		if(key_press('i')) panel_edit_mode_change(panel, edit_mode_insert);
		if(key_press('I'))
		{
			panel_cursor_move_start(panel);
			panel_edit_mode_change(panel, edit_mode_insert);
		}

		if(key_press('a'))
		{
			panel_cursor_move_right(panel);
			panel_edit_mode_change(panel, edit_mode_insert);
		}

		if(key_press('A'))
		{
			panel_cursor_move_end(panel);
			panel_edit_mode_change(panel, edit_mode_insert);
		}

		/* Movement */
		if(key_press('k')) panel_cursor_move_up(panel);
		if(key_press('j')) panel_cursor_move_down(panel);
		if(key_press('h')) panel_cursor_move_left(panel);
		if(key_press('l')) panel_cursor_move_right(panel);

		if(key_press('s')) panel_cursor_move_start(panel);
		if(key_press('e')) panel_cursor_move_end(panel);

		if(key_press('w')) panel_cursor_move_word_next(panel);
		if(key_press('b')) panel_cursor_move_word_previous(panel);

		/* Lines */
		if(key_press('o')) panel_line_add_bellow(panel);
		//if(key_press('O')) buffer_add_line_above(buffer);

	} break;
	case edit_mode_insert:
	{
		/* Return to normal mode */
		if(key_press(KEY_ESCAPE)) panel_edit_mode_change(panel, edit_mode_normal);

		/* Type test */
		for(u32 i = ' '; i < '~'; ++i)
		{
			if(key_press(i))
			{
				panel_line_input(panel, i);
			}
		}
	} break;
	default: break;
	}
}

void
panel_update(struct Panel *panel)
{
	camera_update(&editor->camera);
	panel_input(panel);
}

void
window_draw(struct Window *window)
{
	for(u32 i = 0; i < window->panel_count; ++i)
	{
		struct Panel *panel = &window->panels[i];
		panel_draw(panel);
	}
}

void
window_update(struct Window *window)
{
	for(u32 i = 0; i < window->panel_count; ++i)
	{
		struct Panel *panel = &window->panels[i];
		panel_update(panel);
	}
}

void
editor_update(void)
{
	for(u32 i = 0; i < editor->window_count; ++i)
	{
		struct Window *window = &editor->windows[i];
		window_update(window);
	}
}

void
editor_draw(void)
{
	struct Rec2 cursor = REC2(V2_ZERO, SCREEN_DIM);
	rec2_draw(&cursor, editor->camera.transform, canvas->z[layer_background], editor->color_background);

	for(u32 i = 0; i < editor->window_count; ++i)
	{
		struct Window *window = &editor->windows[i];
		window_draw(window);
	}

	#if 0
	cstr_draw("TEST", 4, V2(200, 200), &editor->align_bar, canvas->z[layer_cursor], V4_COLOR_WHITE);
	struct Rec2 rec = REC2(V2(200, 200), V2(300, 300));
	rec2_draw(&rec, editor->camera.transform, canvas->z[layer_text], V4_COLOR_RED);
	#endif
}

/* Working on */
