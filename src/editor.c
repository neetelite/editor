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
content_data_shift_update(struct Position *position)
{
	struct PositionPointer pointer = position_pointer_from_position(position);

	struct Content *current = pointer.content;
	struct Content *previous = line_content_get_previous(pointer.line, current);

	f32 scale = 1.0;

	v2 pos = V2_ZERO;
	u32 codepoint_previous;

	/* Get the previous codepoint and the beginning of the last character */
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
	f32 char_height = editor->font.height;
	v2 pos = V2(editor->margin_left, HEIGHT-(line->id*char_height));
	char content[5];

	snprintf(content, 5, "%d", line->id+1);
	cstr_draw(content, (line->id/10)+1, pos, &editor->align_buffer,
		  canvas->z[layer_content_text], V4_COLOR_RED);
}

void
panel_line_draw(struct Panel *panel, struct Line *line)
{

	panel_line_number_draw(panel, line);
	for(u32 i = 0; i < line->content_count; ++i)
	{
		struct Content *content = &line->contents[i];

		if(editor->draw_content_background)
		{

			f32 y = HEIGHT - (editor->font.height * (line->id + 1));
			f32 ml = editor->margin_left;
			f32 nw = editor->line_number_width;
			if(content->char_count != 0)
			{
				v4 color = V4_ZERO;
				if(content->char_count == content->size_alloc) color = V4_COLOR_BLUE; /* FULL */
				else if(content->size == content->size_alloc) color = V4_COLOR_GREEN; /* NOT FULL */
				else color = V4_COLOR_RED; /* SPLIT */

				if(line->id % 2 == 0)
				{
				    if(i % 2 == 0) color = v4_mf(color, 0.5);
				    else color = v4_mf(color, 0.3);
				}
				else
				{
				    if(i % 2 == 0) color = v4_mf(color, 0.3);
				    else color = v4_mf(color, 0.5);
				}

				/* TODO: Put this in shift update */
				struct Visual *visual = content->visual;
				v2 start = V2(visual[0].rec.start.x +ml+nw, y);
				v2 end = V2_ZERO;

				if(line->content_count > 1 && i < line->content_count - 1)
				{
					struct Content *content_next = &line->contents[i+1];
					struct Visual *visual_next = content_next->visual;
					end = V2(visual_next[0].rec.start.x + ml+nw, y+editor->font.height);
				}
				else
				{
					end = V2(visual[content->char_count-1].rec.end.x +ml+nw, y+editor->font.height);
				}

				struct Rec2 content_rec = REC2(start, end);

				rec2_draw(&content_rec, editor->camera.transform,
					canvas->z[layer_content_background], color);
			}
		}

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
panel_cursor_move_up(struct Panel *panel)
{
	if(panel->pos.y <= 0) return;

	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	struct Line *line_above = buffer_line_get_by_id(pointer.buffer, panel->pos.y-1);
	panel_cursor_move_to_line(panel, line_above);

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
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	if(panel->pos.y >= pointer.buffer->line_count-1) return;

	struct Line *line_below = buffer_line_get_by_id(pointer.buffer, panel->pos.y+1);
	panel_cursor_move_to_line(panel, line_below);

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

	f32 start_y = HEIGHT - (editor->font.height * (line->id + 1));
	f32 padding = editor->line_number_width + editor->margin_left;

	v2 start = V2_ZERO;;
	v2 end = V2_ZERO;;

	if(panel->pos.x == 0)
	{
		start = V2(padding, start_y);
		end = v2_a(start, cursor_dim);
	}
	else
	{
		if(panel->pos.c == EOL)
		{
			struct Content *content_last = line_content_get_last(line);
			struct Rec2 *rec_last = &content_last->visual[content_last->char_count-1].rec;
			start = V2(padding+rec_last->end.x, start_y);
			end = v2_a(start, cursor_dim);
		}
		else
		{
			/* TODO: Cursor width is the size of character width */
			/* You need to get the previous content for that */

			/* NOTE: You cannot get the current character becaues it may not be typed yet! */
			struct Content *content = line_content_get_by_id(line, panel->pos.c);
			if(panel->pos.i == 0)
			{
				struct Content *content_prev = line_content_get_previous(line, content);
				struct Rec2 *rec = &content_prev->visual[content_prev->char_count-1].rec;
				start = V2(padding+rec->end.x, start_y);
				end = v2_a(start, cursor_dim);
			}
			else
			{
				struct Rec2 *rec = &content->visual[panel->pos.i-1].rec;
				start = V2(padding+rec->end.x, start_y);
				end = v2_a(start, cursor_dim);
			}
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
	/* NOTE: This function uses a goto to make it easier to read */

	/* TODO CLEAN: The lower part of the function can be made more readable (remove the add_before) */

	if(panel->pos.c == EOL)
	{
		/* Add at the start */
		if(panel->pos.x == 0) line_content_add_start(&panel->pos);

		/* Add at the end */
		else
		{
			struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
			struct Content *content_last = line_content_get_last(pointer.line);

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

	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);
	if(content_is_full(pointer.content))
	{
		/* Add before the content */
		if(panel->pos.i == 0)
		{
			/* Check weather we can use a previous content instead of creating a new one */
			if(panel->pos.c != 0)
			{
				    struct Content *content_prev =
					    line_content_get_previous(pointer.line, pointer.content);

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
	/* Edit Mode */
	cstr_draw(edit_mode_str_table[panel->edit_mode], EDIT_MODE_STR_SIZE,
		  V2(0, 0), &editor->align_bar, canvas->z[layer_content_text], V4_COLOR_WHITE);
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
	font_init(&editor->font, "ibm.ttf", "IBM Plex Mono", 24, "ascii");
	//font_init(&editor->font, "arial.ttf", "Arial", 24, "ascii");
	//font_init(&editor->font, "roboto.ttf", "Arial", 24, "ascii");

	editor->align_bar    = ALIGN("left", "bottom");
	editor->align_buffer = ALIGN("left", "top");
	editor->line_number_width = 20.0;
	editor->margin_left = 5.0;
	editor->color_background = v4_mf(V4_COLOR_WHITE, 0.05);
	gl_viewport_color_set(editor->color_background);
	#if 0
	editor->content_min = 128;
	editor->content_max = 128;
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
		if(char_is_alpha(*pointer_curr.c) == false &&
		   char_is_alpha(*pointer_prev.c) == true)
		{
			panel->pos = pos_prev;
			return;
		}

		/* Beginning of line */
		if(pos_curr.x == 0 && char_is_alpha(*pointer_curr.c))
		{
			panel->pos = pos_curr;
			return;
		}

		pos_prev = pos_curr;
		pointer_prev = pointer_curr;
	}
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
		if(pos_curr.x == 0 && char_is_alpha(*pointer_curr.c))
		{
			panel->pos = pos_curr;
			return;
		}

		/* Beginning of word */
		if(char_is_alpha(*pointer_curr.c) == true &&
		   char_is_alpha(*pointer_prev.c) == false)
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
}

void
panel_line_add_below(struct Panel *panel)
{
	struct PositionPointer pointer = position_pointer_from_position(&panel->pos);

	panel->pos.y += 1;
	panel_line_add(panel);
}

void
panel_input(struct Panel *panel)
{
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

				/* Top Right */
				case key_o: panel_line_add_below(panel); break;
				};
			}
			else
			{
				switch(key)
				{
				/* Middle Row */
				case key_h: panel_cursor_move_start(panel); break;
				case key_l: panel_cursor_move_end(panel); break;

				case key_j: /* TODO: */; break;
				case key_k: /* TODO: */; break;

				case key_f: panel_cursor_move_word_next(panel, false); break;
				case key_d: panel_cursor_move_word_prev(panel, false); break;

				/* Top Left */
				case key_w: panel_edit_mode_change(panel, edit_mode_visual_line); break;
				case key_e: panel_edit_mode_change(panel, edit_mode_replace); break;

				/* Top Right */
				case key_o: panel_line_add_above(panel); break;
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

		/* Process input */
		for(u32 key = 0; key < KEY_MAX; ++key)
		{
			if(key_press(key) == false) continue;

			if(0) {}
			else if(key == key_tab);
			else if(key == key_enter) panel_line_add_below(panel);
			else
			{
				/* Enter characters */
				char c = ascii_from_key(key);
				if(c != '\0') panel_line_input(panel, c);
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

/* Working on */
