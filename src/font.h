#ifndef _FONT_H
#define _FONT_H

struct Font_Glyph
{
	u32 codepoint;

	//Image bitmap;
	struct GL_Texture texture;

	Vec2 align_percentage;
};

struct Font_Info
{
	u32 glyph_count;
	u32 glyph_max;

	/* LEARN: Are these supposed to be here */
	u32 codepoint_max_plus_one;

	f32 ascender_height;
	f32 descender_height;
	f32 external_leading;
};

struct Asset_Font
{
	struct Font_Info info;
	struct Font_Glyph *glyphs;

	f32 *kerning_table;
	u16 *unicode_map;

	u32 bitmap_id_offset;

	u32 height;
};

int font_init(struct Asset_Font *font, CString TTFFileName, CString FontName, u32 PixelHeight, CString charset_name);
void font_free(struct Asset_Font *font);
f32 font_kerning_get(struct Asset_Font *font, u32 codepoint, u32 codepoint_previous);
Image font_glyph_bitmap_get(struct Asset_Font *font, u32 codepoint);
Vec2 font_glyph_alignment_get(struct Asset_Font *font, u32 codepoint);

#endif /* _FONT_H */
