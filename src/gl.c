/* Init */
void
gl_program_text()
{
	struct GL_ProgramText *program = &gl->program_text;
	struct GL_Shader shaders_text[2] = {GL_SHADER_VERTEX, "quad.vs", GL_SHADER_FRAGMENT, "text.fs"};
	program->handle = gl_program_create(shaders_text, 2);

	program->location_mvp = gl_uniform_location(program->handle, "u_mvp");
	program->location_color = gl_uniform_location(program->handle, "u_color");
}

void
gl_program_quad(void)
{
	struct GL_ProgramQuad *program = &gl->program_quad;
	struct GL_Shader shaders_quad[2] = {GL_SHADER_VERTEX, "quad.vs", GL_SHADER_FRAGMENT, "quad_color.fs"};
	program->handle = gl_program_create(shaders_quad, 2);

	program->location_mvp = gl_uniform_location(program->handle, "u_mvp");
	program->location_rec = gl_uniform_location(program->handle, "u_rec");
	program->location_color = gl_uniform_location(program->handle, "u_color");
}

void
gl_program_texture(void)
{
	struct GL_ProgramTexture *program = &gl->program_texture;
	struct GL_Shader shaders_texture[2] = {GL_SHADER_VERTEX, "quad.vs", GL_SHADER_FRAGMENT, "quad_texture.fs"};
	program->handle = gl_program_create(shaders_texture, 2);

	program->location_mvp = gl_uniform_location(program->handle, "u_mvp");
	program->location_rec = gl_uniform_location(program->handle, "u_rec");
}

void
gl_projection_2d()
{
	/* Projection */
	f32 w = os_context.dim.x;
	f32 h = os_context.dim.y;

	f32 z = 1.0;
	//f32 z = 1.0;
	//f32 n = 0.00001; /* NOTE(lungu): Can't be negative or zero */
	//f32 f = 1000;
	f32 n = 0.00001; /* NOTE(lungu): Can't be negative or zero */
	f32 f = 10.0;

	f32 r = w;
	f32 l = 0;
	f32 t = h;
	f32 b = 0;

	f32 A = (2 / (r - l)) / z;
	f32 B = (2 / (t - b)) / z;
	f32 C = -2 / (f - n);

	f32 D = -(r + l) / (r - l);
	f32 E = -(t + b) / (t - b);
	f32 F = 0;

	gl->projection_2d = MAT4
		(
			A, 0, 0, D,
			0, B, 0, E,
			0, 0, C, F,
			0, 0, 0, 1
		);
}

void
gl_init(void)
{
	gl = &app->gl;

	/* TODO(lungu): Depth test doesn't work sometimes, it has to do with the graphics card it's using */

	gl_viewport_rec_set(V4(0, 0, os_context.dim.x, os_context.dim.y));
	gl_viewport_color_set(v4_mf(V4_COLOR_WHITE, 0.05));

	/* Programs */
	gl_program_text();
	gl_program_quad();
	gl_program_texture();
	gl_projection_2d();

	gl_enable(GL_TEXTURE_2D);
	gl_enable(GL_DEPTH_TEST);
	gl_enable(GL_BLEND);

	gl_func_depth(GL_LEQUAL);

	gl_func_blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//gl_func_blend(GL_SRC_ALPHA, GL_SRC_COLOR);
}
