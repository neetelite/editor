void
box2_draw(struct Box2 *box, mat4 mat_view, f32 z, v4 color)
{
	struct GL_ProgramQuad *program = &gl->program_quad;
	gl_program_bind(program->handle);

	v3 pos = V3(box->pos.x, box->pos.y, z);
	v3 dim = V3(box->dim.x, box->dim.y, 0.0);
	mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, V3_ZERO));
	gl_uniform_mat4(program->location_mvp, mvp);
	gl_uniform_v4(program->location_color, color);

	u32 triangle_count = 2;
	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);

	gl_program_unbind();
}

void
rec2_draw(struct Rec2 *rec, mat4 mat_view, f32 z, v4 color)
{
	struct Box2 box = box2_from_rec2(*rec);
	box2_draw(&box, mat_view, z, color);
}