struct Aesthetic
{
	/* NOTE: Transparency 0.0 if you don't want to draw them */

	v4 foreground;
	v4 background;

	v4 underline;
	v4 overline;
	v4 crossline;

	v4 cursor_foreground;
	v4 cursor_background;

	v4 shadow;
	v2 shadow_offset;

	v4 contour;
	f32 contour_size;
};

struct Aesthetic aesthetics[token_count];
