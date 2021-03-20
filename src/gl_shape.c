void
box2_draw(struct Box2 *box, Mat4 mat_view, f32 z, Vec4 color)
{
	struct GL_ProgramQuad *program = &gl->program_quad;
	gl_program_bind(program->handle);

	Vec3 pos = VEC3(box->pos.x, box->pos.y, z);
	Vec3 dim = VEC3(box->dim.x, box->dim.y, 0.0);
	Mat4 mvp = mat4_m(mat_view, mat4_transform3(pos, dim, VEC3_ZERO));
	gl_uniform_mat4(program->location_mvp, mvp);
	gl_uniform_vec4(program->location_color, color);

	u32 triangle_count = 2;
	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);

	gl_program_unbind();
}

void
rec2_draw(struct Rec2 *rec, Mat4 mat_view, f32 z, Vec4 color)
{
	struct Box2 box = box2_from_rec2(*rec);
	box2_draw(&box, mat_view, z, color);
}
