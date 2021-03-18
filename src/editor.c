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

v2
panel_line_offset_gen(struct Panel *panel, struct Line *line)
{
	f32 tab = editor->tab_size * editor->space_size * line->indent;
	f32 offset_x = (editor->line_number_width + editor->margin_left + tab) + panel->screen.pos.x;
	f32 offset_y = (HEIGHT - (editor->font.height * (1 + line->id))) + panel->screen.pos.y;

	v2 result = V2(offset_x, offset_y);
	return(result);
}

struct PositionPointer
position_pointer_from_position(struct Position *position)
{
	/* NOTE: Now that we return NULL pointers on get_by_id() function
	   we need to check if those before using them */

	/* TODO NOTE: This function could be written better, it's quite ambiguous */
	/* We do assignment inside of if() parenthesis */

	struct PositionPointer result = {0};
	if(position->b == EOF ||
	   (result.buffer = editor_buffer_get_by_id(position->b)) == NULL)
	{
		result.buffer = NULL;
		result.line = NULL;
		result.content = NULL;
		result.c = NULL;
		return(result);
	}

	if(position->y == EOF ||
	   (result.line = buffer_line_get_by_id(result.buffer, position->y)) == NULL)
	{
		result.line = NULL;
		result.content = NULL;
		result.c = NULL;
		return(result);
	}

	if(position->c == EOL ||
	   (result.content = line_content_get_by_id(result.line, position->c)) == NULL)

	{
		result.content = NULL;
		result.c = NULL;
		return(result);
	}
	else
	{
		result.c = &result.content->data[position->i];
	}

	return(result);
}

void
position_print(struct Position *position)
{
	printf("B: %d\n", position->b);
	printf("Y: %d\n", position->y);
	printf("X: %d\n", position->x);
	printf("C: %d\n", position->c);
	printf("I: %d\n", position->i);
	printf("\n");
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
	result.data = mem_alloc(size_total, true); /* TODO: Make it false in release */
	result.visual = (struct Visual *)((u08 *)result.data+size_data);

	return(result);
}

void
line_content_rec_update(struct Line *line, struct Content *content)
{
	if(content->char_count == 0)
	{
		content->rec = REC2(V2_ZERO, V2_ZERO);
		return;
	}

	/* Rec */
	if(content->id == 0)
	{
		v2 start = V2(content->visual[0].rec.start.x, 0);
		v2 end = V2(content->visual[content->char_count-1].rec.end.x, editor->font.height);
		content->rec = REC2(start, end);

	}
	else
	{
		struct Content *prev = line_content_get_by_id(line, content->id - 1);
		v2 start = V2(prev->rec.end.x, 0);
		v2 end = v2_a(start, V2(content->visual[content->char_count-1].rec.end.x, editor->font.height));
		content->rec = REC2(start, end);
	}
}

void
content_data_shift_update(struct Position *position)
{
	struct PositionPointer ptr = position_pointer_from_position(position);
	struct Content *content = ptr.content;

	f32 scale = 1.0;

	v2 pos = V2_ZERO;
	v2 min, max;
	u32 codepoint_previous = 0;

	/* TODO: */
	//for(u32 i = position->i; i < content->char_count; ++i)
	for(u32 i = 0; i < content->char_count; ++i)
	{
		struct Visual *visual = &content->visual[i];
		u32 codepoint = content->data[i];

		v2 advance = V2_ZERO;
		advance.x = scale*font_kerning_get(&editor->font, codepoint, codepoint_previous);
		pos.x += advance.x;

		struct GL_Texture texture = font_glyph_texture_get(&editor->font, codepoint);

		v2 baseline_align = font_glyph_alignment_get(&editor->font, codepoint);
		baseline_align = v2_m(baseline_align, V2(texture.image.width, texture.image.height));

		v2 render_pos = v2_s(pos, baseline_align);

		/* TODO: Don't use the current codepoint, use a space codepoint maybe? */
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

	line_content_rec_update(ptr.line, content);
}

void
content_data_shift_right(struct Position *pos, u32 n)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	for(u32 i = pointer.content->char_count+n-1; i > pos->i; --i)
	{
		u32 id_from = i-n;
		u32 id_to = i;
		pointer.content->data[id_to] = pointer.content->data[id_from];
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

	v3 pos = V3(box.pos.x, box.pos.y, canvas->z[layer_content_text]);
	v3 dim = V3(box.dim.x, box.dim.y, 0.0);
	mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, V3_ZERO));
	gl_uniform_mat4(program->location_mvp, mvp);

	u32 triangle_count = 2;
	gl_vao_draw(0, triangle_count * 3, GL_TRIANGLES);

	gl_texture_unbind(GL_TEXTURE_2D);
	gl_program_unbind();
}


struct Token *
line_token_from_pos(struct Position *pos)
{
	struct Token* result = NULL;
	struct PositionPointer ptr = position_pointer_from_position(pos);
	for(i32 i = 0; i < ptr.line->token_count; ++i)
	{
		struct Token *curr = &ptr.line->tokens[i];
		if(i == ptr.line->token_count-1)
		{
			result = curr;
			break;
		}

		struct Token *next = &ptr.line->tokens[i+1];
		if(curr->pos <= pos->x && pos->x < next->pos)
		{
			result = curr;
			break;
		}
	}
	return(result);
}

void
content_text_draw(struct Position *position, struct Content *content, v2 offset)
{
	offset = v2_a(offset, content->rec.start);

	mat4 mat_view = editor->camera.transform;
	v4 color = V4_COLOR_WHITE;

	struct GL_ProgramText *program = &gl->program_text;
	gl_program_bind(program->handle);

	struct Position pos = *position;
	for(u32 i = 0; i < content->char_count; ++i)
	{
		pos.c = content->id;
		pos.x = content->char_start + i;
		pos.i = i;

		struct Token *token = line_token_from_pos(&pos);
		struct Aesthetic *aes = aesthetic_from_token(token);

		v4 color;
		if(aes != NULL && aes->foreground.a != 0.0) color = aes->foreground;
		else color = V4_COLOR_WHITE;
		gl_uniform_v4(program->location_color, color);

		struct Visual *visual = &content->visual[i];
		u32 codepoint = content->data[i];

		struct GL_Texture texture = font_glyph_texture_get(&editor->font, codepoint);
		gl_texture_bind(&texture, GL_TEXTURE_2D);

		v2 min = v2_a(visual->rec.start, offset);
		v2 max = v2_a(visual->rec.end, offset);

		struct Rec2 rec = REC2(min, max);
		struct Box2 box = box2_from_rec2(rec);

		v3 pos = V3(box.pos.x, box.pos.y, canvas->z[layer_content_text]);
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
void
line_init(struct Line *line)
{
	line->content_max = 1;
	line->contents = mem_alloc(line->content_max * sizeof(*line->contents), false);
}

struct Line
line_new(u32 id)
{
	struct Line result = {0};
	result.id = id;
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
line_content_shift_update(struct Position *position)
{
	/* This function updates char_counts and content data */

	struct Position pos = *position;
	struct PositionPointer ptr = position_pointer_from_position(&pos);

	/* From current content to last content */
	for(u32 content_id = position->c; content_id < ptr.line->content_count; ++content_id)
	{

		/* Update char start */
		struct Content *content = line_content_get_by_id(ptr.line, content_id);
		content->id = content_id; /* NOTE: You need to do this, YES */

		if(content->id == 0)
		{
			content->char_start = 0;
		}
		else
		{
			struct Content *content_prev = line_content_get_by_id(ptr.line, content->id - 1);
			content->char_start = content_char_end(content_prev);
		}

		/* Set the correct content and update it */
		pos.c = content->id;
		if(content->id != position->c) pos.i = 0;

		content_data_shift_update(&pos);
	}

	/* Update line char count */
	struct Content *content_last = line_content_get_last(ptr.line);
	ptr.line->char_count = content_char_end(content_last);

	/* Tokens */
	/* TODO: */
	//pos = *position;
	//line_tokenize(&pos);

	/* TODO: Should I do a line REC for the visuals? */
	//struct Content *content_first = ;
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
		u32 id_from = content_id - n;
		u32 id_to   = content_id;
		pointer.line->contents[id_to] = pointer.line->contents[id_from];
	}
}

void
line_content_add_start(struct Position *pos)
{
	/* NOTE: Only use for start of line */
	ASSERT(pos->x == 0);

	struct Buffer *buffer = editor_buffer_get_by_id(pos->b);
	struct Line *line = buffer_line_get_by_id(buffer, pos->y);

	ASSERT(line->content_max == 0)
	line_init(line);
	line->content_count += 1;

	struct Content *content = line_content_get_first(line);
	*content = content_new(0, 0, 0);

	pos->c = 0;
	pos->i = 0;
}

void
line_content_add_end(struct Position *pos)
{
	/* NOTE: Only use for end of line */
	ASSERT(pos->c == EOL);
	ASSERT(pos->i == EOL);

	struct Buffer *buffer = editor_buffer_get_by_id(pos->b);
	struct Line *line = buffer_line_get_by_id(buffer, pos->y);

	if(line->content_count+1 > line->content_max) line_grow(line);

	struct Content *content_last = line_content_get_last(line);
	struct Content *content = &line->contents[line->content_count];
	*content = content_new(content_last->id+1, content_char_end(content_last), 0);
	line->content_count += 1;

	pos->c = content->id;
	pos->i = 0;
}

void
line_content_add_before(struct Position *pos)
{
	struct Buffer *buffer = editor_buffer_get_by_id(pos->b);
	struct Line *line = buffer_line_get_by_id(buffer, pos->y);

	if(line->content_count+1 > line->content_max) line_grow(line);
	line_content_shift_right(pos, 1);

	struct Content *content = line_content_get_by_id(line, pos->c);
	*content = content_new(pos->c, content->char_start, 0);

	line->content_count += 1;
}

void
line_content_add_between(struct Position *pos)
{
	/* TODO: Change this to char_index */
	struct PositionPointer pointer = position_pointer_from_position(pos);
	ASSERT(content_is_full(pointer.content) == true);

	/* NOTE: if content_max = 1, we need to do this twice */
	//if(pointer.line->content_count+2 > pointer.line->content_max)
	while(pointer.line->content_count+2 > pointer.line->content_max)
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

	struct Content *left = pointer.content+0;
	struct Content *middle = pointer.content+1;
	struct Content *right = pointer.content+2;

	u32 initial_size = left->size;

	left->id = pos->c;
	left->char_start = left->char_start;
	left->char_count = pos->i;
	left->size = left->char_count;
	left->size_alloc = left->size_alloc;
	left->data = left->data;
	left->visual = left->visual;

	*middle = content_new(pos->c + 1, content_char_end(left), 0);

	right->id = left->id + 2;
	right->char_start = middle->char_start;
	right->char_count = initial_size - left->char_count;
	right->size = right->char_count;
	right->size_alloc = left->size_alloc;
	right->data = left->data + left->size;
	right->visual = left->visual + left->size;

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
	f32 pos_x = editor->margin_left + panel->screen.pos.x;
	f32 pos_y = (HEIGHT - (editor->font.height * line->id)) + panel->screen.pos.y;
	v2 pos = V2(pos_x, pos_y);

	char content[5];
	if(line->id < 10) snprintf(content, 5, " %d", line->id);
	else snprintf(content, 5, "%d", line->id);

	cstr_draw(content, 5, pos, &editor->align_buffer, canvas->z[layer_content_text], V4_COLOR_RED);
}

void
panel_line_draw(struct Panel *panel, struct Line *line)
{
	panel_line_number_draw(panel, line);
	v2 offset = panel_line_offset_gen(panel, line);

	struct Position pos = panel->pos;
	pos.y = line->id;
	for(u32 i = 0; i < line->content_count; ++i)
	{
		struct Content *content = &line->contents[i];
		if(content->char_count == 0) continue;

		struct Rec2 content_rec = {0};
		content_rec.start = v2_a(content->rec.start, offset);
		content_rec.end = v2_a(content->rec.end, offset);
		if(rec2_overlap(&content_rec, &panel->screen.rec) == false) continue;

		/* Background */
		if(editor->draw_content_background)
		{
			v4 color = V4_ZERO;
			if(content->char_count == content->size_alloc) color = V4_COLOR_BLUE; /* FULL */
			else if(content->size == content->size_alloc) color = V4_COLOR_GREEN; /* NOT FULL */
			else color = V4_COLOR_RED; /* SPLIT */

			if(line->id % 2 == 0)
			{
			    if(content->id % 2 == 0) color = v4_mf(color, 0.5);
			    else color = v4_mf(color, 0.3);
			}
			else
			{
			    if(content->id % 2 == 0) color = v4_mf(color, 0.3);
			    else color = v4_mf(color, 0.5);
			}

			rec2_draw(&content_rec, editor->camera.transform, canvas->z[layer_content_background], color);
		}

		/* Text */
		content_text_draw(&pos, content, offset);
	}
}

/* Buffer */
struct Buffer
buffer_new(void)
{
	struct Buffer result = {0};
	result.line_count = 1;
	result.line_max = 1;
	result.lines = mem_alloc(result.line_max * sizeof(*result.lines), true);
	result.lines[0] = line_new(0);
	return(result);
}

void
panel_cursor_move_to_line(struct Panel *panel, struct Line *line)
{
	struct Content *content = NULL;
	if(panel->pos.x_min_active == true)
	{
		if(panel->pos.x_min <= line->char_count)
		{
			content = line_content_get_by_char_pos(line, panel->pos.x_min);
			panel->pos.x = panel->pos.x_min;
		}
		else
		{
			panel->pos.x = line->char_count;
		}
	}
	else
	{
		if(panel->pos.x >= line->char_count)
		{
			panel->pos.x_min_active = true;
			panel->pos.x_min = panel->pos.x;
			panel->pos.x = line->char_count;
		}
		else
		{
			content = line_content_get_by_char_pos(line, panel->pos.x);
			panel->pos.x = panel->pos.x;
		}

	}

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

	panel->pos.y = line->id;
}

void
screen_update_rec(struct Screen *screen)
{
	//screen->rec = REC2(screen->pos, screen->dim);
	screen->rec = rec2_sort(screen->rec);
}

void
panel_screen_update_up(struct Panel *panel)
{
	f32 lines_of_spacing = 5;
	f32 cursor_pos = (panel->pos.y * editor->font.height);
	f32 stop_at = (panel->screen.pos.y) + (lines_of_spacing * editor->font.height);
	if(cursor_pos > stop_at) return;

	f32 first_line = 0;
	f32 new_screen_y = cursor_pos - (lines_of_spacing * editor->font.height);
	if(new_screen_y < first_line) new_screen_y = first_line;

	panel->screen.pos.y = new_screen_y;
	screen_update_rec(&panel->screen);
}

void
panel_screen_update_down(struct Panel *panel)
{
	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);

	f32 lines_of_spacing = 5;
	f32 cursor_pos = panel->pos.y * editor->font.height;
	f32 stop_at = (panel->screen.pos.y + HEIGHT) - (lines_of_spacing * editor->font.height);
	if(cursor_pos < stop_at) return;

	f32 last_line = (buffer->line_count) * editor->font.height;
	f32 new_screen_y = (cursor_pos - HEIGHT) + (lines_of_spacing * editor->font.height);
	if(new_screen_y+HEIGHT > last_line) new_screen_y = last_line-HEIGHT;

	panel->screen.pos.y = new_screen_y;
	screen_update_rec(&panel->screen);
}

void
panel_screen_update(struct Panel *panel)
{
	panel_screen_update_up(panel);
	panel_screen_update_down(panel);
}

void
panel_cursor_move_to_line_by_id(struct Panel *panel, u32 id)
{
	if(id < 0) return;

	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(id >= ptr.buffer->line_count) return;

	struct Line *line = buffer_line_get_by_id(ptr.buffer, id);

	u32 y_tmp = panel->pos.y;
	panel_cursor_move_to_line(panel, line);

	#if 1
	if(id < y_tmp) panel_screen_update_up(panel);
	else panel_screen_update_down(panel);
	#else
	panel_screen_update(panel);
	#endif
}

void
panel_cursor_move_up(struct Panel *panel)
{
	if(panel->pos.y <= 0) return;

	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);

	struct Line *line_above = buffer_line_get_by_id(ptr.buffer, panel->pos.y-1);
	panel_cursor_move_to_line(panel, line_above);
	panel_screen_update_up(panel);

	if(editor->print_movement_info)
	{
		printf("UP\n");
		position_print(&panel->pos);
		printf("\n");
	}
}


void
panel_cursor_move_down(struct Panel *panel)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(panel->pos.y >= ptr.buffer->line_count-1) return;

	struct Line *line_below = buffer_line_get_by_id(ptr.buffer, panel->pos.y+1);
	panel_cursor_move_to_line(panel, line_below);
	panel_screen_update_down(panel);

	if(editor->print_movement_info)
	{
		printf("DOWN\n");
		position_print(&panel->pos);
		printf("\n");
	}
}

void
panel_cursor_move_right(struct Panel *panel)
{
	panel->pos.x_min_active = false;

	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	if(panel->pos.x >= pointer.line->char_count) return;

	if(panel->pos.x+1 >= pointer.line->char_count)
	{
		panel->pos.c = EOL;
		panel->pos.i = EOL;
	}
	else
	{
		u32 char_end = content_char_end(pointer.content);
		if(panel->pos.x >= char_end-1)
		{
			struct Content *content_next = line_content_get_next(pointer.line, pointer.content);
			panel->pos.c = content_next->id;
			panel->pos.i = 0;
		}
		else
		{
			panel->pos.c = panel->pos.c;
			panel->pos.i += 1;
		}
	}
	panel->pos.x += 1;

	if(editor->print_movement_info)
	{
		printf("RIGHT\n");
		position_print(&panel->pos);
		printf("\n");
	}
}

void
panel_cursor_move_left(struct Panel *panel)
{
	panel->pos.x_min_active = false;

	if(panel->pos.x <= 0) return;

	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	struct Line *line = buffer_line_get_by_id(buffer, panel->pos.y);

	if(panel->pos.c == EOL)
	{
		struct Content *content_last = line_content_get_last(line);
		panel->pos.c = content_last->id;
		panel->pos.i = content_last->char_count-1;
	}
	else
	{
		struct Content *content_cur = line_content_get_by_id(line, panel->pos.c);
		if(panel->pos.x <= content_cur->char_start)
		{
			struct Content *content_prev = line_content_get_previous(line, content_cur);
			panel->pos.c = content_prev->id;
			panel->pos.i = content_prev->char_count-1;
		}
		else
		{
			panel->pos.c = panel->pos.c;
			panel->pos.i -= 1;
		}
	}

	panel->pos.x -= 1;

	if(editor->print_movement_info)
	{
		printf("LEFT\n");
		position_print(&panel->pos);
		printf("\n");
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

	if(editor->print_movement_info)
	{
		printf("START\n");
		position_print(&panel->pos);
		printf("\n");
	}
}

//void
//cursor_move_end(struct Position *pos)

void
panel_cursor_move_end(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.c = -1;
	panel->pos.i = -1;
	panel->pos.x = pointer.line->char_count;

	if(editor->print_movement_info)
	{
		printf("END\n");
		position_print(&panel->pos);
		printf("\n");
	}
}

void
panel_cursor_move_prev_empty_line(struct Panel *panel, bool check_indent_zero)
{
	/* TODO: */
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	for(i32 line_id = ptr.line->id-1; line_id >= 0; --line_id)
	{
		struct Line *line = buffer_line_get_by_id(ptr.buffer, line_id);
		if(line->char_count == 0)
		{
			if(check_indent_zero == false)
			{
				panel_cursor_move_to_line(panel, line);
				break;
			}
			else if(line->indent == 0)
			{
				panel_cursor_move_to_line(panel, line);
				break;
			}
		}
	}
	panel_screen_update_up(panel);
}

void
panel_cursor_move_next_empty_line(struct Panel *panel, bool check_indent_zero)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	for(i32 line_id = ptr.line->id+1; line_id < ptr.buffer->line_count; ++line_id)
	{
		struct Line *line = buffer_line_get_by_id(ptr.buffer, line_id);
		if(line->char_count == 0)
		{
			if(check_indent_zero == false)
			{
				panel_cursor_move_to_line(panel, line);
				break;
			}
			else if(line->indent == 0)
			{
				panel_cursor_move_to_line(panel, line);
				break;
			}
		}
	}

	panel_screen_update_down(panel);
}

void
panel_cursor_move_first_line(struct Panel *panel)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	struct Line *line = buffer_line_get_by_id(ptr.buffer, 0);
	panel_cursor_move_to_line(panel, line);
	panel_screen_update_up(panel);
}

void
panel_cursor_move_last_line(struct Panel *panel)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	struct Line *line = buffer_line_get_by_id(ptr.buffer, ptr.buffer->line_count-1);
	panel_cursor_move_to_line(panel, line);
	panel_screen_update_down(panel);
}

struct Screen
screen_new()
{
	struct Screen result = {0};
	result.pos = V2_ZERO;
	result.dim = SCREEN_DIM;

	#if 0
	result.rec = REC2(V2(0, HEIGHT/4), V2(WIDTH, HEIGHT-(HEIGHT/4)));
	#else
	result.rec = REC2(result.pos, result.dim);
	#endif
	result.rec = rec2_sort(result.rec);

	return(result);
}

void
panel_screen_move_up(struct Panel *panel)
{
	panel->screen.pos.y -= editor->font.height;

	f32 stop_at = -HEIGHT + editor->font.height;
	if(panel->screen.pos.y < stop_at) panel->screen.pos.y = stop_at;

	screen_update_rec(&panel->screen);

	if(editor->print_movement_info)
	{
		printf("PANEL UP\n");
		printf("Y: %f\n", panel->screen.pos.y);
		printf("\n");
	}
}

void
panel_screen_move_down(struct Panel *panel)
{
	panel->screen.pos.y += editor->font.height;

	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);

	f32 stop_at = (buffer->line_count-1) * editor->font.height;
	if(panel->screen.pos.y > stop_at) panel->screen.pos.y = stop_at;

	screen_update_rec(&panel->screen);

	if(editor->print_movement_info)
	{
		printf("PANEL DOWN\n");
		printf("Y: %f\n", panel->screen.pos.y);
		printf("\n");
	}
}

/* TODO: Change this to panel */
void
panel_cursor_draw(struct Panel *panel)
{
	struct Buffer *buffer = editor_buffer_get_by_id(0);
	struct Line *line = buffer_line_get_by_id(buffer, panel->pos.y);

	/* TODO: We need to find the exact char_width */
	f32 char_width = 10;
	f32 char_height = editor->font.height;
	v2 cursor_dim = V2(char_width, char_height);
	v2 offset = panel_line_offset_gen(panel, line);

	v2 start = V2_ZERO;
	v2 end = V2_ZERO;

	if(panel->pos.x == 0)
	{
		start = V2(0, 0);
		end = v2_a(start, cursor_dim);
	}
	else
	{
		if(panel->pos.c == EOL)
		{
			struct Content *content_last = line_content_get_last(line);
			start = V2(content_last->rec.end.x, 0);
			end = v2_a(start, cursor_dim);
		}
		else
		{
			/* TODO: Cursor width is the size of character width */
			struct Content *content_curr = line_content_get_by_id(line, panel->pos.c);
			if(panel->pos.i == 0)
			{
				struct Content *content_prev = line_content_get_previous(line, content_curr);
				start = V2(content_prev->rec.end.x, 0);
				end = v2_a(start, cursor_dim);
			}
			else
			{
				/* NOTE: You cannot get the current character becaues it may not be typed yet! */
				struct Rec2 *rec = &content_curr->visual[panel->pos.i-1].rec;
				start = V2(content_curr->rec.start.x + rec->end.x, 0);
				end = v2_a(start, cursor_dim);
			}
		}
	}

	start = v2_a(start, offset);
	end = v2_a(end, offset);

	struct Rec2 cursor = REC2(start, end);
	rec2_draw(&cursor, editor->camera.transform, canvas->z[layer_cursor], V4_COLOR_RED);

	/* TODO: Text */
	//content_char_draw(pointer.content, panel->pos.i, editor->color_background);
}

void
position_update_prev_char(struct Position *pos, struct PositionPointer *ptr)
{
	bool had_to_change_content = true;

	if(pos->x == 0)
	{
		/* Find a previous line with content */
		loop
		{
			/* Buffer has no content */
			if(ptr->line->id == 0)
			{
				ptr->content = EOL_PTR;
				ptr->c = EOL_PTR;
				return;
			}

			pos->y -= 1;
			ptr->line = buffer_line_get_by_id(ptr->buffer, pos->y);

			/* We found a good line */
			if(ptr->line->char_count) {ptr->content = line_content_get_last(ptr->line); break;}
		}
	}
	else if(pos->c == EOF) ptr->content = line_content_get_last(ptr->line);
	else if(pos->i == 0) ptr->content = line_content_get_by_id(ptr->line, ptr->content->id-1);
	else had_to_change_content = false;

	if(had_to_change_content)
	{
		ASSERT(ptr->content->char_count > 0);
		pos->x = content_char_end(ptr->content)-1;
		pos->c = ptr->content->id;
		pos->i = ptr->content->char_count-1;

		ptr->c = &ptr->content->data[pos->i];
	}
	else
	{
		pos->x -= 1;
		pos->i -= 1;

		ptr->c = &ptr->content->data[pos->i];
	}

	if(editor->print_movement_info)
	{
		printf("CHAR PREV\n");
		position_print(pos);
		printf("\n");
	}
}

void
position_update_next_char(struct Position *pos, struct PositionPointer *ptr)
{
	bool had_to_change_content = true;

	if(pos->x+1 >= ptr->line->char_count)
	{
		/* Find a next line with content */
		loop
		{
			/* Buffer has no more contents */
			if(ptr->line->id == ptr->buffer->line_count-1)
			{
				pos->x += 1;
				pos->c = EOL;
				pos->i = EOL;

				ptr->content = EOL_PTR;
				ptr->c = EOL_PTR;
				return;
			}

			pos->y += 1;
			ptr->line = buffer_line_get_by_id(ptr->buffer, pos->y);

			/* We found a good line */
			if(ptr->line->char_count)
			{
				ptr->content = line_content_get_first(ptr->line);
				break;
			}
		}
	}
	else if(pos->i == ptr->content->char_count-1)
	{
		ptr->content = line_content_get_by_id(ptr->line, ptr->content->id+1);
	}
	else had_to_change_content = false;

	if(had_to_change_content)
	{
		pos->x = ptr->content->char_start;
		pos->c = ptr->content->id;
		pos->i = 0;

		ptr->c = &ptr->content->data[pos->i];
	}
	else
	{
		pos->x += 1;
		pos->i += 1;
		ptr->c = &ptr->content->data[pos->i];
	}

	if(editor->print_movement_info)
	{
		printf("CHAR NEXT\n");
		position_print(pos);
		printf("\n");
	}
}

void
content_data_shift_left(struct Position *pos, u32 n)
{
	ASSERT(n == 1);

	struct PositionPointer ptr = position_pointer_from_position(pos);
	if((i32)ptr.content->char_count-1 - pos->i <= n) return;

	u32 end_id = ptr.content->char_count-1;
	u32 start_id = pos->i;
	for(u32 i = start_id; i < end_id; ++i)
	{
		u32 id_from = i+1;
		u32 id_to = i;
		ptr.content->data[id_to] = ptr.content->data[id_from];
	}
}

void
panel_remove_char(struct Panel *panel)
{
	if(panel->pos.i == EOL) return;

	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(ptr.content->char_count == 0) return;

	content_data_shift_left(&panel->pos, 1);
	line_content_shift_update(&panel->pos);

	if(panel->pos.i == ptr.content->char_count-1)
	{
		if(ptr.content->id == ptr.line->content_count-1)
		{
			panel->pos.c = EOF;
			panel->pos.i = EOF;
		}
		else
		{
			panel->pos.c += 1;
			panel->pos.i = 0;
		}
	}

	ptr.content->char_count -= 1;
	ptr.line->char_count -= 1;

	line_content_rec_update(ptr.line, ptr.content);
}

void
panel_backspace(struct Panel *panel)
{
	if(panel->pos.x == 0) return;
	panel_cursor_move_left(panel);
	panel_remove_char(panel);
}

void
panel_insert_char(struct Panel *panel, char c)
{
	/* NOTE: This function uses a goto to make it easier to read */

	/* TODO CLEAN: The lower part of the function can be made more readable (remove the add_before) */

	if(panel->pos.c == EOL)
	{
		struct PositionPointer ptr = position_pointer_from_position(&panel->pos);

		/* Add at the start */
		if(panel->pos.x == 0)
		{
			if(ptr.line->content_count == 0)
			{
				line_content_add_start(&panel->pos);
			}
			else
			{
				panel->pos.c = 0;
				panel->pos.i = 0;
			}
		}

		/* Add at the end */
		else
		{
			struct Content *content_last = line_content_get_last(ptr.line);

			/* Check weather or not we can use the last content instead of creating a new one */
			u32 size_end = content_size_end(content_last);
			if(panel->pos.x >= size_end) line_content_add_end(&panel->pos);
			else
			{
				panel->pos.c = content_last->id;
				panel->pos.i = content_last->char_count;
			}
		}
		goto insert_character;
	}

	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(content_is_full(ptr.content))
	{
		/* Add before the content */
		if(panel->pos.i == 0)
		{
			/* Check weather we can use a previous content instead of creating a new one */
			if(panel->pos.c != 0)
			{
				    struct Content *content_prev =
					    line_content_get_previous(ptr.line, ptr.content);

				    u32 size_end = content_size_end(content_prev);
				    if(panel->pos.x < size_end)
				    {
					    panel->pos.c = content_prev->id;
					    panel->pos.i = content_prev->char_count;
				    }
				    else line_content_add_before(&panel->pos);
			}

			/* Otherwise just make a new one */
			else line_content_add_before(&panel->pos);

		}

		/* Add in the middle of the content, my spliting the content in two */
		else line_content_add_between(&panel->pos);
	}

insert_character:
	line_content_input(&panel->pos, c);
	panel_cursor_move_right(panel);
}

void
panel_screen_background_draw(struct Panel *panel)
{
	struct Screen *screen = &panel->screen;
	v4 color = v4_mf(V4_COLOR_WHITE, 0.05);
	rec2_draw(&screen->rec, gl->projection_2d, canvas->z[layer_screen_background], color);
}

void
panel_buffer_draw(struct Panel *panel)
{
	/* TODO SPEED: */
	buffer_tokenize(panel);

	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	panel_screen_background_draw(panel);

	i32 line_start = (panel->screen.pos.y / editor->font.height) - 1;
	i32 line_end = ((panel->screen.pos.y + HEIGHT) / editor->font.height);

	if(line_start < 0) line_start = 0;
	if(line_end > buffer->line_count) line_end = buffer->line_count;

	for(u32 i = line_start; i < line_end; ++i)
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

	result.screen = screen_new();

	/* Position */
	if(buffer == NULL)
	{
		result.pos.b = -1;
		result.pos.y = -1;
		result.pos.c = -1;
		result.pos.x = -1;
		result.pos.i = -1;
		return(result);
	}

	result.pos.b = buffer->id;
	if(buffer->lines == NULL)
	{
		result.pos.y = -1;
		result.pos.c = -1;

		result.pos.x = -1;
		result.pos.i = -1;
		return(result);
	}

	result.pos.y = 0;
	result.pos.x = 0;
	if(buffer->lines[0].contents == NULL)
	{
		result.pos.c = -1;
		result.pos.i = -1;
		return(result);
	}

	result.pos.c = 0;
	result.pos.i = 0;
	return(result);
}

void
panel_bar_draw(struct Panel *panel)
{
	f32 reduce = -5;
	struct Rec2 rec = REC2(V2(0, 0), V2(WIDTH, editor->font.height + reduce));
	rec2_draw(&rec, gl->projection_2d, canvas->z[layer_bar_background], V4_COLOR_WHITE);

	/* Edit Mode */
	cstr_draw(edit_mode_str_table[panel->edit_mode], EDIT_MODE_STR_SIZE,
		  V2(0, 0), &editor->align_bar, canvas->z[layer_bar_text], V4_COLOR_BLACK);
}

void
panel_draw(struct Panel *panel)
{
	panel_buffer_draw(panel);
	panel_bar_draw(panel);
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
	result.panel_count = 1;
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
	f32 font_size = 30;
	font_init(&editor->font, "ibm.ttf", "IBM Plex Mono", font_size, "ascii");
	//font_init(&editor->font, "arial.ttf", "Arial", font_size, "ascii");
	//font_init(&editor->font, "roboto.ttf", "Arial", font_size, "ascii");

	editor->align_bar    = ALIGN("left", "bottom");
	editor->align_buffer = ALIGN("left", "top");

	editor->line_number_width = 50.0;
	editor->margin_left = 5.0;

	editor->space_size = 10.0;
	editor->tab_size = 8;

	editor->color_background = v4_mf(V4_COLOR_WHITE, 0.8);
	gl_viewport_color_set(editor->color_background);
	#if 1
	editor->content_min = 128;
	editor->content_max = 128;
	#else
	editor->content_min = 4;
	editor->content_max = 4;
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
	editor->buffers[0].language = language_c;

	/* Window */
	editor->window_id = 0;
	editor->window_count = 1;
	editor->windows = mem_alloc(editor->window_count * sizeof(*editor->windows), true);
	editor->windows[0] = window_new(&editor->buffers[0]);

	/* Language */
	lang_c_keyword_init();

	/* Aesthetics */
	aesthetics_init();
}

void
panel_cursor_move_word_prev(struct Panel *panel, bool stop_at_token)
{
	struct Position pos_init = panel->pos;
	struct Position pos_curr = panel->pos;
	struct Position pos_prev = pos_curr;

	struct PositionPointer pointer_curr = position_pointer_from_position(&panel->pos);
	struct PositionPointer pointer_prev = pointer_curr;

	/* NOTE: Unlike the move_word_next function we need to skip a char, because of how the cursor words */
	position_update_prev_char(&pos_curr, &pointer_curr);
	pos_prev = pos_curr;
	pointer_prev = pointer_curr;

	loop
	{
		position_update_prev_char(&pos_curr, &pointer_curr);

		/* Beginning of buffer */
		if(pointer_prev.c == NULL)
		{
			panel->pos = pos_init;
			return;
		}

		/* Special characters */
		if(stop_at_token)
		{
			for(u32 i = 0; i < COUNT(word_tokens_stop_at); ++i)
			{
				if(*pointer_prev.c == word_tokens_stop_at[i])
				{
					panel->pos = pos_prev;
					return;
				}
			}
		}

		if(pointer_curr.c == NULL)
		{
			panel->pos = pos_init;
			return;
		}


		/* Beginning of word */
		if(char_is_alphanumeric(*pointer_curr.c) == false &&
		   char_is_alphanumeric(*pointer_prev.c) == true)
		{
			panel->pos = pos_prev;
			return;
		}

		/* Beginning of line */
		if(pos_curr.x == 0 && char_is_alphanumeric(*pointer_curr.c))
		{
			panel->pos = pos_curr;
			return;
		}

		pos_prev = pos_curr;
		pointer_prev = pointer_curr;
	}

	panel_screen_update_up(panel);
}

void
panel_cursor_move_word_next(struct Panel *panel, bool stop_at_token)
{
	struct Position pos_curr = panel->pos;
	struct Position pos_prev = pos_curr;

	struct PositionPointer pointer_curr = position_pointer_from_position(&panel->pos);
	struct PositionPointer pointer_prev = pointer_curr;

	loop
	{
		position_update_next_char(&pos_curr, &pointer_curr);

		/* End of buffer */
		if(pos_curr.x >= pointer_prev.line->char_count &&
		   pos_curr.y+1 >= pointer_prev.buffer->line_count)
		{
			return;
		}

		/* Beginning of line */
		if(pos_curr.x == 0 && char_is_alphanumeric(*pointer_curr.c))
		{
			panel->pos = pos_curr;
			return;
		}

		/* Beginning of word */
		if(char_is_alphanumeric(*pointer_curr.c) == true &&
		   char_is_alphanumeric(*pointer_prev.c) == false)
		{
			panel->pos = pos_curr;
			return;
		}

		/* Special characters */
		if(stop_at_token)
		{
			for(u32 i = 0; i < COUNT(word_tokens_stop_at); ++i)
			{
				if(*pointer_curr.c == word_tokens_stop_at[i])
				{
					panel->pos = pos_curr;
					return;
				}
			}
		}

		pos_prev = pos_curr;
		pointer_prev = pointer_curr;
	}

	panel_screen_update_down(panel);
}

void
buffer_line_shift_down_update(struct Position *pos, u32 n)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	struct Position line_pos = {0};
	line_pos.b = pos->b;

	for(i32 line_id = pos->y; line_id < pointer.buffer->line_count; line_id += 1)
	{
		if(pointer.buffer->lines[line_id].content_count != 0)
		{
			line_pos.y = line_id;
			line_content_shift_update(&line_pos);
		}
	}
}

void
buffer_line_shift_down(struct Position *pos, u32 n)
{
	struct PositionPointer pointer = position_pointer_from_position(pos);
	for(u32 i = pointer.buffer->line_count+n-1; i > pos->y; --i)
	{
		u32 id_from = i-n;
		u32 id_to = i;
		pointer.buffer->lines[id_to] = pointer.buffer->lines[id_from];
		pointer.buffer->lines[id_to].id = id_to;
	}
}

void
buffer_line_shift_update(struct Position *pos)
{
	struct PositionPointer ptr = position_pointer_from_position(pos);
	struct Position line_pos = {0};
	line_pos.b = pos->b;

	for(i32 line_id = pos->y; line_id < ptr.buffer->line_count; line_id += 1)
	{
		struct Line *line = buffer_line_get_by_id(ptr.buffer, line_id);
		line->id = line_id;
	}
}

void
buffer_line_shift_up(struct Position *pos, u32 n)
{
	if(n <= 0) return;

	struct PositionPointer ptr = position_pointer_from_position(pos);
	if(pos->y+n >= ptr.buffer->line_count) return;

	//for(u32 i = pointer.buffer->line_count+n-1; i > pos->y; --i)
	for(u32 i = pos->y; i < ptr.buffer->line_count-n; ++i)
	{
		u32 id_from = i+n;
		u32 id_to = i;
		ptr.buffer->lines[id_to] = ptr.buffer->lines[id_from];
	}
}

void
panel_line_add(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	if(pointer.buffer->line_count+1 >= pointer.buffer->line_max)
	{
		buffer_grow(pointer.buffer);
		pointer = position_pointer_from_position(&panel->pos);
	}

	/* NOTE: We cannot increment buffer->line_count before buffer_line_shift_down */
	if(panel->pos.y < pointer.buffer->line_count)
	{
		buffer_line_shift_down(&panel->pos, 1);

		pointer.buffer->line_count += 1;
		buffer_line_shift_down_update(&panel->pos, 1);
	}
	else
	{
		pointer.buffer->line_count += 1;
	}
	pointer.buffer->lines[panel->pos.y] = line_new(panel->pos.y);

	panel->pos.x = 0;
	panel->pos.c = EOL;
	panel->pos.i = EOL;
}

void
panel_line_add_above(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	panel_line_add(panel);
	panel_screen_update_up(panel);
}

void
panel_line_add_below(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.y += 1;
	panel_line_add(panel);
	panel_screen_update_down(panel);
}

void
panel_line_indent_right(struct Panel *panel, u32 line_id)
{
	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	struct Line *line = buffer_line_get_by_id(buffer, line_id);

	line->indent += 1;
}

void
panel_line_indent_left(struct Panel *panel, u32 line_id)
{
	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	struct Line *line = buffer_line_get_by_id(buffer, line_id);
	if(line->indent == 0) return;

	line->indent -= 1;
}

void
panel_cursor_move_to_char(struct Panel *panel, u32 char_id)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	panel->pos.x = char_id;

	struct Content *content = line_content_get_by_char_pos(ptr.line, panel->pos.x);
	panel->pos.c = content->id;
	panel->pos.i = content_char_index_from_pos(content, panel->pos.x);
}

void
panel_cursor_mark(struct Panel *panel)
{
	panel->mark.pos = panel->pos;
}

void
panel_cursor_move_to_mark(struct Panel *panel)
{
	panel->pos = panel->mark.pos;
}

void
buffer_write_path(struct Buffer *buffer, String path)
{
	/* LEARN: Is it better to append to a file, or append to a buffer and then write to a file? */

	/* Write to a data buffer first */
	u64 file_size = 0;
	for(i32 i = 0; i < buffer->line_count; ++i)
	{
		/* NOTE: +1 is a '\n' */
		/* TODO: File indent, each indent is a '\t' */
		struct Line *line = &buffer->lines[i];
		file_size += line->indent;
		file_size += line->char_count;
		if(line->id != buffer->line_count-1) file_size += 1; /* newline */
	}

	char *file_data = mem_alloc(file_size, false);
	Byte *at = (Byte *)file_data;

	for(i32 line_id = 0; line_id < buffer->line_count; ++line_id)
	{
		struct Line *line =  &buffer->lines[line_id];
		for(i32 i = 0; i < line->indent; ++i) buf_push_u8(&at, '\t');
		for(i32 content_id = 0; content_id < line->content_count; content_id += 1)
		{
			struct Content *content = &line->contents[content_id];
			if(content->char_count == 0) continue;

			buf_push_bytes(&at, content->data, content->char_count);
		}
		if(line->id != buffer->line_count-1) buf_push_u8(&at, '\n');
	}

	/* Write to file */
	struct File file = file_init(path);
	file_open(&file, file_open_mode_byte_write);

	file_write(&file, file_data, file_size);

	file_close(&file);
	mem_free(file_data);
}

void
buffer_free(struct Buffer *buffer)
{
	for(u32 line_id = 0; line_id < buffer->line_count; line_id += 1)
	{
		struct Line *line = &buffer->lines[line_id];
		for(u32 content_id = 0; content_id < line->content_count; content_id += 1)
		{
			struct Content *content = &line->contents[content_id];
			if(content->data) mem_free(content->data);
		}
		if(line->contents) mem_free(line->contents);
	}
	mem_free(buffer->lines);
	*buffer = (struct Buffer){0};
}

void
buffer_reset(struct Buffer *buffer)
{
	/* TODO: Check if it actually needs to be freed */
	buffer_free(buffer);
	*buffer = buffer_new();
}

i32
buffer_read_path(struct Buffer *buffer, String path, struct Panel *panel_out)
{
	/* Read from file */
	struct File file = file_init(path);
	file_open(&file, file_open_mode_byte_read);

	u64 filesize = file_size(&file);
	if(filesize == 0)
	{
		FATAL("This shouldn't be happening!\n");
		file_close(&file);
		return(-1);
	}

	char *file_data = mem_alloc(filesize, false);

	file_read(&file, file_data, filesize);
	file_close(&file);

	/* Read into buffer */
	buffer_reset(buffer);

	struct Line *first_line = buffer_line_get_by_id(buffer, 0);
	line_init(first_line);
	first_line->content_count += 1;

	struct Content *first_content = line_content_get_by_id(first_line, 0);
	*first_content = content_new(0, 0, 0);

	struct Panel panel = panel_new(buffer);
	for(u32 i = 0; i < filesize; ++i)
	{
		char *at = &file_data[i];

		if(i >= filesize - 45)
		{
			u32 breakpoint = 0;
		}

		switch(*at)
		{
		case '\t':
		{
			panel_line_indent_right(&panel, panel.pos.y);
		} break;
		case '\r': break;
		case '\n':
		{
			panel_line_add_below(&panel);
			panel_cursor_move_down(&panel);
			panel_cursor_move_start(&panel);
		} break;
		default:
		{
			panel_insert_char(&panel, *at);
		} break;
		}
	}

	mem_free(file_data);

	panel.screen = screen_new();
	panel.pos.x = 0;
	panel.pos.y = 0;
	panel.pos.c = 0;
	panel.pos.i = 0;
	panel.pos.x_min_active = false;
	panel.pos.x_min = 0;

	*panel_out = panel;

	buffer_tokenize(&panel);

	return(0);
}

void
content_free(struct Content *content)
{
	if(content->data != NULL) mem_free(content->data);
	*content = (struct Content){0};
}

void
panel_line_clear(struct Panel *panel, u32 line_id)
{
	/* TODO: Should I free the data in contents? */

	struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
	struct Line *line = buffer_line_get_by_id(buffer, line_id);
	for(i32 i = 0; i < line->content_count; ++i)
	{
		struct Content *content = line_content_get_by_id(line, i);
		content_free(content);
	}

	line->char_count = 0;
	line->content_count = 0;
	line->content_max = 0;
	if(line->contents != NULL) mem_free(line->contents);
	line->contents = NULL;

	if(panel->pos.y == line_id)
	{
		panel->pos.x = 0;
		panel->pos.c = EOL;
		panel->pos.i = EOL;
	}
}

void
panel_line_remove(struct Panel *panel, u32 line_id)
{
	/* NOTE: This function doesn't free the line */
	struct Position pos = panel->pos;
	struct Buffer *buffer = editor_buffer_get_by_id(pos.b);
	if(buffer->line_count == 1) return;

	pos.y = line_id;

	buffer_line_shift_up(&pos, 1);
	buffer_line_shift_update(&pos);

	if(panel->pos.y == buffer->line_count-1) panel_cursor_move_up(panel);
	buffer->line_count -= 1;
}

void
panel_line_delete(struct Panel *panel, u32 line_id)
{
	struct Position pos = panel->pos;
	pos.y = line_id;

	struct Buffer *buffer = editor_buffer_get_by_id(pos.b);
	if(buffer->line_count == 1)
	{
		panel_line_clear(panel, line_id);
		return;
	}

	struct Line *line = buffer_line_get_by_id(buffer, pos.y);
	if(line->content_max) mem_free(line->contents);

	panel_line_remove(panel, line_id);
}

void
panel_line_join_above(struct Panel *panel)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(panel->pos.y <= 0) return;

	if(ptr.line->content_count == 0)
	{
		panel_line_delete(panel, ptr.line->id);
		panel_cursor_move_up(panel);
		panel_screen_update(panel);
		return;
	}

	struct Line *line_above = buffer_line_get_by_id(ptr.buffer, ptr.line->id-1);
	if(line_above->content_count == 0)
	{
		panel_line_delete(panel, line_above->id);
		panel_cursor_move_up(panel);
		panel_screen_update(panel);
		return;
	}

	u64 new_content_count = ptr.line->content_count + line_above->content_count;
	while(ptr.line->content_max < new_content_count)
	{
		line_grow(ptr.line);
		ptr = position_pointer_from_position(&panel->pos);
	}

	u64 line_content_size = ptr.line->content_count * sizeof(struct Content);
	Byte *new_content_data = (Byte *)ptr.line->contents + line_content_size;

	u64 line_above_content_size = line_above->content_count * sizeof(struct Content);
	mem_cpy(new_content_data, line_above->contents, line_above_content_size);
	ptr.line->content_count = new_content_count;

	line_content_shift_update(&panel->pos);

	panel_line_remove(panel, line_above->id);
	panel_cursor_move_up(panel);
	panel_screen_update(panel);
}

void
panel_line_join_below(struct Panel *panel)
{
	struct PositionPointer ptr = position_pointer_from_position(&panel->pos);
	if(panel->pos.y >= ptr.buffer->line_count-1) return;

	if(ptr.line->content_count == 0)
	{
		panel_line_delete(panel, ptr.line->id);
		panel_screen_update(panel);
		return;
	}

	struct Line *line_below = buffer_line_get_by_id(ptr.buffer, ptr.line->id+1);
	if(line_below->content_count == 0)
	{
		panel_line_delete(panel, line_below->id);
		panel_screen_update(panel);
		return;
	}

	u64 new_content_count = ptr.line->content_count + line_below->content_count;
	while(ptr.line->content_max < new_content_count)
	{
		line_grow(ptr.line);
		ptr = position_pointer_from_position(&panel->pos);
	}

	u64 line_content_size = ptr.line->content_count * sizeof(struct Content);
	Byte *new_content_data = (Byte *)ptr.line->contents + line_content_size;

	u64 line_below_content_size = line_below->content_count * sizeof(struct Content);
	mem_cpy(new_content_data, line_below->contents, line_below_content_size);
	ptr.line->content_count = new_content_count;

	line_content_shift_update(&panel->pos);

	panel_line_remove(panel, line_below->id);
	panel_screen_update(panel);
}

void
panel_input(struct Panel *panel)
{
	/* All Modes */
	if(key_alt_down())
	{
		struct Buffer *buffer = editor_buffer_get_by_id(panel->pos.b);
		String path_read = STR("./test_read.txt");
		String path_write = STR("./test_write.txt");

		if(0) {}
		else if(key_press(key_w)) buffer_write_path(buffer, path_write);
		else if(key_press(key_r)) buffer_read_path(buffer, path_read, panel);
		else if(key_press(key_j)) panel_screen_move_down(panel);
		else if(key_press(key_k)) panel_screen_move_up(panel);
		return;
	}


	/* Specific Mode */
	switch(panel->edit_mode)
	{
	case edit_mode_normal:
	{
		for(u32 key = 0; key < KEY_MAX; ++key)
		{
			if(key_press(key) == false) continue;

			if(key_shift_up())
			{
				switch(key)
				{
				/* Middle Row */
				case key_h: panel_cursor_move_left(panel); break;
				case key_j: panel_cursor_move_down(panel); break;
				case key_k: panel_cursor_move_up(panel); break;
				case key_l: panel_cursor_move_right(panel); break;

				case key_f: panel_cursor_move_word_next(panel, true); break;
				case key_d: panel_cursor_move_word_prev(panel, true); break;

				/* Top Left */
				case key_w: panel_edit_mode_change(panel, edit_mode_visual); break;
				case key_e: panel_edit_mode_change(panel, edit_mode_insert); break;
				case key_a: panel_cursor_move_to_mark(panel); break;

				/* Top Right */
				case key_i: panel_line_indent_right(panel, panel->pos.y); break;
				case key_o: panel_line_add_below(panel); break;
				case key_p: panel_line_join_below(panel); break;

				/* Bottom Left */
				case key_x: panel_remove_char(panel); break;

				/* Bottom Right */
				case key_comma: panel_cursor_move_prev_empty_line(panel, true); break;
				case key_period: panel_cursor_move_next_empty_line(panel, true); break;
				};
			}
			else
			{
				switch(key)
				{
				/* Middle Row */
				case key_h: panel_cursor_move_start(panel); break;
				case key_l: panel_cursor_move_end(panel); break;

				case key_j: panel_cursor_move_next_empty_line(panel, false); break;
				case key_k: panel_cursor_move_prev_empty_line(panel, false); break;

				case key_f: panel_cursor_move_word_next(panel, false); break;
				case key_d: panel_cursor_move_word_prev(panel, false); break;
				case key_a: panel_cursor_mark(panel); break;

				/* Top Left */
				case key_w: panel_edit_mode_change(panel, edit_mode_visual_line); break;
				case key_e: panel_edit_mode_change(panel, edit_mode_replace); break;

				/* Top Right */
				case key_i: panel_line_indent_left(panel, panel->pos.y); break;
				case key_o: panel_line_add_above(panel); break;
				case key_p: panel_line_join_above(panel); break;

				/* Bottom Left */
				case key_x: panel_line_delete(panel, panel->pos.y); break;

				/* Bottom Right */
				case key_comma: panel_cursor_move_first_line(panel); break;
				case key_period: panel_cursor_move_last_line(panel); break;
				}
			}
		}
	} break;
	case edit_mode_command:
	{
		if(key_press(key_escape)) panel_edit_mode_change(panel, edit_mode_normal);
	}break;
	case edit_mode_insert:
	{
		/* Return to normal mode */
		if(key_press(key_escape)) panel_edit_mode_change(panel, edit_mode_normal);

		if(key_alt_down() || key_ctrl_down()) break;

		/* Process input */
		for(u32 key = 0; key < KEY_MAX; ++key)
		{
			if(key_press(key) == false) continue;

			switch(key)
			{
			case key_tab:
			{
				if(key_shift_down()) panel_line_indent_left(panel, panel->pos.y);
				else panel_line_indent_right(panel, panel->pos.y);
			} break;
			case key_enter: panel_line_add_below(panel); break;
			case key_backspace: panel_backspace(panel); break;
			default:
			{
				/* Enter characters */
				char c = ascii_from_key(key);
				if(c != '\0') panel_insert_char(panel, c);
			}
			}
		}
	} break;
	case edit_mode_replace:
	{
		if(key_press(key_escape)) panel_edit_mode_change(panel, edit_mode_normal);
	}break;
	case edit_mode_visual:
	{
		if(key_press(key_escape)) panel_edit_mode_change(panel, edit_mode_normal);
	}break;
	case edit_mode_visual_line:
	{
		if(key_press(key_escape)) panel_edit_mode_change(panel, edit_mode_normal);
	}break;
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
	/* Debug */
	#if BUILD_DEBUG
	if(key_press(key_f9)) bool_toggle(&editor->draw_content_background);
	if(key_press(key_f10)) bool_toggle(&editor->print_movement_info);
	#endif

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
	rec2_draw(&rec, editor->camera.transform, canvas->z[layer_content_text], V4_COLOR_RED);
	#endif
}
