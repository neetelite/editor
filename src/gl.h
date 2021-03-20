/* File specific to application */
struct GL_ProgramText
{
	u32 handle;
	u32 location_mvp;
	u32 location_color;
};

struct GL_ProgramQuad
{
	u32 handle;
	u32 location_mvp;
	u32 location_rec;
	u32 location_color;
};

struct GL_ProgramTexture
{
	u32 handle;
	u32 location_mvp;
	u32 location_rec;
};

struct GL
{
	struct GL_ProgramText program_text;
	struct GL_ProgramQuad program_quad;
	struct GL_ProgramTexture program_texture;

	Mat4 projection_2d;
};

struct GL *gl;

#define Asset_Texture GL_Texture
