f32
text_width(CString text, struct Asset_Font *font)
{
	/* TODO(lungu): Crash when text is null string */

	f32 result = 0.0;

	u32 codepoint_previous = 0;
	for(i32 i = 0; text[i] != '\0'; ++i)
	{
		u32 codepoint = text[i];

		f32 advance = font_kerning_get(font, codepoint, codepoint_previous);
		result += advance;

		codepoint_previous = codepoint;
	}

	result += font_glyph_texture_get(font, codepoint_previous).image.width - 2;

	return(result);
}

void
cstr_draw_full(struct Asset_Font *font, CString text, u32 len,
	       v2 pos, struct Alignment *alignment, mat4 mat_view, f32 z, v4 color)
{
	/* Horizontal Alignment */
	f32 width = text_width(text, font);

	if(0) {}
	else if(alignment->horizontal == align_horizontal_left) pos.x -= 0;
	else if(alignment->horizontal == align_horizontal_middle) pos.x -= width*0.5;
	else if(alignment->horizontal == align_horizontal_right) pos.x -= width;

	/* TODO(lungu): font->height doesn't seem to work very well */
	if(0) {}
	else if(alignment->vertical == align_vertical_top) pos.y -= (font->height);
	else if(alignment->vertical == align_vertical_center) pos.y -= (font->height)*0.5;
	else if(alignment->vertical == align_vertical_bottom) pos.y -= 0;

	f32 scale = 1.0;

	/* OpenGL */
	struct GL_ProgramText *program = &gl->program_text;
	gl_program_bind(program->handle);

	gl_uniform_mat4(program->location_mvp, mat_view);
	gl_uniform_v4(program->location_color, color);

	u32 codepoint_previous = 0;
	for(Index i = 0; i < len && text[i] != '\0'; ++i)
	{
		u32 codepoint = text[i];

		f32 advance_x = scale*font_kerning_get(font, codepoint, codepoint_previous);
		pos.x += advance_x;

		struct GL_Texture texture = font_glyph_texture_get(font, codepoint);
		gl_texture_bind(&texture, GL_TEXTURE_2D);

		v2 baseline_align = font_glyph_alignment_get(font, codepoint);
		baseline_align = v2_m(baseline_align, V2(texture.image.width, texture.image.height));

		v2 render_pos = v2_s(pos, baseline_align);

		v2 min, max;
		min = render_pos;
		max = v2_a(render_pos, V2(texture.image.width, texture.image.height));

		struct Rec2 rec = REC2(min, max);
		struct Box2 box = box2_from_rec2(rec);

		v3 pos = V3(box.pos.x, box.pos.y, z);
		v3 dim = V3(box.dim.x, box.dim.y, 0.0);
		mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, V3_ZERO));
		gl_uniform_mat4(program->location_mvp, mvp);

		u32 triangle_count = 2;
		gl_vao_draw(0, triangle_count * 3, GL_TRIANGLES);

		codepoint_previous = codepoint;
	}

	gl_texture_unbind(GL_TEXTURE_2D);
	gl_program_unbind();
}

void
v3_draw_full(struct Asset_Font *font,
	     v3 vec, CString s, v2 pos, struct Alignment *alignment, mat4 mat_view, f32 z, v4 color)
{
	#define BUFFER_MAX 256

	char text[BUFFER_MAX];
	snprintf(text, BUFFER_MAX, "%s x: %-12f y: %-12f z: %-12f", s, vec.x, vec.y, vec.z);

	cstr_draw_full(font, text, BUFFER_MAX, pos, alignment, mat_view, z, color);
}
