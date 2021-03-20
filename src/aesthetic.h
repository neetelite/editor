struct Aesthetic
{
	/* NOTE: Transparency 0.0 if you don't want to draw them */

	Vec4 foreground;
	Vec4 background;

	Vec4 underline;
	Vec4 overline;
	Vec4 crossline;

	Vec4 cursor_foreground;
	Vec4 cursor_background;

	Vec4 shadow;
	Vec2 shadow_offset;

	Vec4 contour;
	f32 contour_size;
};

struct Aesthetic aesthetics[token_count];
