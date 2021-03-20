#define STB_TRUETYPE_IMPLEMENTATION
#include "../inc/stb_truetype.h"

typedef struct GlyphResult
{
	Vec2  align_percentage;
	f32 kerning_change;
	f32 char_advance;

	u32 width;
	u32 height;
	u32 *pixels;
} GlyphResult;

typedef struct CodepointMask
{
	u32 codepoint_max_plus_one;
	u32 glyph_count;
	u32 *codepoint_from_glyph;
} CodepointMask;


internal i32
glyph_load(GlyphResult *result, stbtt_fontinfo *font_info,  u32 codepoint, Vec2 max_glyph_dim,
	   f32 scale, void *memory_out)
{
	i32 width, height, offset_x, offset_y;
	/* LEARN: - Why do we pass scale twice?? */
	u08 *FontBits = stbtt_GetCodepointBitmap(font_info, scale, scale, (i32)codepoint,
						&width, &height, &offset_x, &offset_y);

	f32 kerning_change = 0;
	Vec2 align_percentage = VEC2(0.5f, 0.5f);
	f32 char_advance = 0;

	if(!(((width > 0) && (width <= (max_glyph_dim.x - 2))) &&
		 ((height > 0) && (height <= (max_glyph_dim.y - 2)))))
	{
	#if 0
		fprintf(stderr,
			"ERROR: Glyph (codepoint: %u) bound out of range, got a %ux%u with a max dim of %ux%u\n",
			codepoint, width, height, max_glyph_dim.x, max_glyph_dim.y);
	#endif
	}

	u32 Bpp = 4;

	u32 out_width  = width  + 2;
	u32 out_height = height + 2;
	u32 out_pitch  = out_width * Bpp;

	memset(memory_out, 0, out_height * out_pitch);

	result->width = out_width;
	result->height = out_height;
	result->pixels = (u32 *)memory_out;

	/* NOTE - Start on the first line and first horizontal space */
	u08 *dst_row = (u08 *)memory_out + (out_height * out_pitch) - out_pitch;
	u08 *src_row = FontBits;
	for(i32 y = 0; y < height; ++y)
	{
		u08 *src = (u08 *)src_row;
		u32 *dst = (u32 *)dst_row + 1;
		for(i32 x = 0; x < width; ++x)
		{
			u32 pixel = *src;
			u32 gray = (pixel & 0xFF);
			*dst++ = ((gray << 24) | 0x00FFFFFF);
			++src;
		}

		dst_row -= out_pitch;
		src_row += width;
	}

	i32 advance, LSB;
	stbtt_GetCodepointHMetrics(font_info, (i32)codepoint, &advance, &LSB);
	char_advance = (f32)advance * scale + offset_x;

	kerning_change = (f32)LSB * scale;

	align_percentage = VEC2(1.0f / (f32)out_width,
				(1.0f + (f32)(offset_y + height)) / (f32)out_height);

	// TODO(michiel): Move the allocation to the outside code and use MakeCodepointBitmap instead
	stbtt_FreeBitmap(FontBits, 0);

	result->align_percentage = align_percentage;
	result->kerning_change = kerning_change;
	result->char_advance = char_advance;

	return(0);
}

internal i32
font_extract(struct Asset_Font *font,
	     CString filename, CString font_name, u32 font_size,
	     CodepointMask *codepoint_mask)
{
	Char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s%s", os_state.path_data, "font/", filename);

	struct Font_Info *info = &font->info;
	info->glyph_count = codepoint_mask->glyph_count;
	info->codepoint_max_plus_one = codepoint_mask->codepoint_max_plus_one;

	u32 *glyph_codepoint = codepoint_mask->codepoint_from_glyph;

	//
	// NOTE(casey): Load and select the requested font
	//

	stbtt_fontinfo font_info_ = {0};
	stbtt_fontinfo *font_info = &font_info_;

	FILE *font_file = fopen(path, "rb");
	if(!font_file)
	{
		fprintf(stderr, "Unable to load font %s from %s\n", font_name, filename);
		return(-1);
	}

	fseek(font_file, 0, SEEK_END);
	size_t font_file_size = ftell(font_file);
	fseek(font_file, 0, SEEK_SET);

	u08 *font_file_buffer = (u08 *)mem_alloc(font_file_size, true);
	size_t font_file_read_size = fread(font_file_buffer, 1, font_file_size, font_file);

	if(font_file_read_size != font_file_size)
	{
		fprintf(stderr, "ERROR: File read incomplete\n");
		return(-2);
	}

	if(stbtt_GetNumberOfFonts(font_file_buffer) != 1)
	{
		fprintf(stderr, "ERROR: Multiple font TTF files not supported\n");
		return(-3);
	}

	// TODO(michiel): Maybe use
	// stbtt_FindMatchingFont(font_file_buffer, font_name, 0);
	if(!stbtt_InitFont(font_info, font_file_buffer, stbtt_GetFontOffsetForIndex(font_file_buffer, 0)))
	{
		fprintf(stderr, "ERROR: Could not initialize stbtt_fontinfo\n");
		return(-4);
	}

	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(font_info, &ascent, &descent, &line_gap);

	f32 scale = stbtt_ScaleForPixelHeight(font_info, font_size);

	i32 min_x, min_y, max_x, max_y;
	stbtt_GetFontBoundingBox(font_info, &min_x, &min_y, &max_x, &max_y);

	Vec2 max_glyph_dim = VEC2(font_size + 2, font_size + 2);

	void *memory_out = mem_alloc(max_glyph_dim.x * max_glyph_dim.y * sizeof(u32), true);

	/* INCOMPLETE - Square function */
	u32 glyphs_size = info->glyph_count * sizeof(struct Font_Glyph);
	u32 kerning_table_size = sizeof(f32) * info->glyph_count * info->glyph_count;
	u32 unicode_map_size = sizeof(u16) * info->codepoint_max_plus_one;

	font->glyphs = (struct Font_Glyph *)mem_alloc(glyphs_size, true);
	font->kerning_table = (f32 *)mem_alloc(kerning_table_size, true);
	font->unicode_map = (u16 *)mem_alloc(unicode_map_size, true);

	memset(font->kerning_table, 0, kerning_table_size);
	memset(font->unicode_map, 0, unicode_map_size);

	// NOTE(casey): Reserve space for the null glyph
	info->ascender_height  = (f32)ascent   * scale;
	info->descender_height = (f32)descent  * scale;
	info->external_leading = (f32)line_gap * scale;

	for(u32 i = 1; i < info->glyph_count; ++i)
	{
		struct Font_Glyph *glyph = font->glyphs + i;

		u32 codepoint = glyph_codepoint[i];

		GlyphResult glyph_info;
		glyph_load(&glyph_info, font_info, codepoint, max_glyph_dim, scale, memory_out);

		struct GL_Texture texture = {0};
		Image *bitmap = &texture.image;

		bitmap->width = glyph_info.width;
		bitmap->height = glyph_info.height;

		bitmap->pitch = bitmap->width * sizeof(u32);
		bitmap->channel_count = 4;

		bitmap->size = bitmap->height * bitmap->pitch;
		bitmap->data = mem_alloc(bitmap->size, true);

		#if 0
		for(i32 y = 0; y < bitmap->height; ++y)
		{
			for(i32 x = 0; x < bitmap->width; ++x)
			{

			}
		}
		#endif
		memcpy(bitmap->data, glyph_info.pixels, bitmap->size);

		/* NOTE: OpenGL */
		gl_texture_gen(&texture, GL_TEXTURE_2D, GL_NEAREST);

		glyph->codepoint = codepoint;
		glyph->texture = texture;
		glyph->align_percentage = glyph_info.align_percentage;

		i32 stb_glyph = stbtt_FindGlyphIndex(font_info, (i32)codepoint);

		/* NOTE: KERNING */
		for(u32 j = 0; j < info->glyph_count; ++j)
		{
			font->kerning_table[i * info->glyph_count + j] +=
				glyph_info.char_advance - glyph_info.kerning_change;

			if(j != 0)
			{
				i32 stb_other_glyph = stbtt_FindGlyphIndex(font_info, (i32)glyph_codepoint[j]);
				f32 kerning = (f32)stbtt_GetGlyphKernAdvance(font_info,
									     stb_other_glyph, stb_glyph) * scale;

				font->kerning_table[j * info->glyph_count + i] += glyph_info.kerning_change + kerning;
				font->unicode_map[glyph->codepoint] = (u16)i;
			}
		}
	}

	#if 0
	/* NOTE - Print out the kerning table to a file */
	fprintf(stdout, "	horizontal_advance = \n		");
	u32 kerning_table_count = info->glyph_count * info->glyph_count;
	for(u32 i = 0; i < kerning_table_count; ++i)
	{
		if(i)
		{
			if((i % 16) == 0)
			{
				fprintf(stdout, ",\n		");
			}
			else
			{
				fprintf(stdout, ", ");
			}
		}

		// TODO(casey): Should we support floating-point advance here?  It looks like
		// we are always actually rounding, so... I guess that's good?
		fprintf(stdout, "%3u", (u32)font->kerning_table[i]);
	}

	fprintf(stdout, ";\n");
	fprintf(stdout, "};\n");
	#endif

	return(0);
}

/* NOTE - Chareacter set functions */
#define CHAR_SET_CREATOR(name) void name(CodepointMask *mask)
typedef CHAR_SET_CREATOR(FontCharsetCreateFunc);

internal void
font_codepoint_include(CodepointMask *mask, u32 E0)
{
	u32 E[] = {E0};
	for(u32 i = 0; i < ARRAY_COUNT(E); ++i)
	{
		u32 codepoint = E[i];
		if(codepoint)
		{
			if(mask->codepoint_from_glyph)
			{
				mask->codepoint_from_glyph[mask->glyph_count] = codepoint;
			}

			if(mask->codepoint_max_plus_one <= codepoint)
			{
				mask->codepoint_max_plus_one = codepoint + 1;
			}

			++mask->glyph_count;
		}
	}
}

internal void
font_codepoint_range_include(CodepointMask *mask, u32 Mincodepoint, u32 Maxcodepoint)
{
	for(u32 Character = Mincodepoint;
		Character <= Maxcodepoint;
		++Character)
	{
		font_codepoint_include(mask, Character);
	}
}

CHAR_SET_CREATOR(ascii)
{
	font_codepoint_include(mask, ' ');
	font_codepoint_range_include(mask, '!', '~');

	// NOTE(casey): Kanji OWL!!!!!!!
	font_codepoint_include(mask, 0x00a9); /* COPYRIGHT */
	font_codepoint_include(mask, 0x5c0f);
	font_codepoint_include(mask, 0x8033);
	font_codepoint_include(mask, 0x6728);
	font_codepoint_include(mask, 0x514e);
}

typedef struct FontCharsetCreateArgs
{
	CString name;
	CString description;
	FontCharsetCreateFunc *function;
} FontCharsetCreateArgs;

global FontCharsetCreateArgs charset_table[] =
{
	{"ascii", "Basic character set for testing font creation and display.", ascii},
};

i32
font_init(struct Asset_Font *font, CString filename,
	  CString font_name, u32 font_height, CString charset_name)
{
	/* NOTE: Examples */
	// "c:/Windows/Fonts/arial.ttf", "Arial", 128
	// "c:/Windows/Fonts/LiberationMono-Regular.ttf", "Liberation Mono", 20

	font->height = font_height;

	FontCharsetCreateArgs *args = 0;

	for(u32 i = 0; i < ARRAY_COUNT(charset_table); ++i)
	{
	FontCharsetCreateArgs *test = charset_table+ i;
	if(strcmp(test->name, charset_name) == 0)
	{
		args = test;
		break;
	}
	}

	if(!args)
	{
		fprintf(stderr, "ERROR: Unrecognized character set \"%s\".\n", charset_name);
		return(-1);
	}

	CodepointMask mask_counter = {0};
	mask_counter.glyph_count = 1;
	args->function(&mask_counter);

	CodepointMask mask = {0};
	mask.glyph_count = 1;
	size_t mask_size = mask_counter.glyph_count * sizeof(u32);
	mask.codepoint_from_glyph = (u32 *)mem_alloc(mask_size, true);
	memset(mask.codepoint_from_glyph, 0, mask_size);

	args->function(&mask);

	font_extract(font, filename, font_name, font_height, &mask);

	return(0);
}

void
font_free(struct Asset_Font *font)
{
	mem_free(font->glyphs);
	mem_free(font->kerning_table);
	mem_free(font->unicode_map);
}

/* NOTE IMPORTANT */

#if 0
u32
GetGlyphFromCodePoint(hha_font *Info, loaded_font *Font, u32 CodePoint)
{
	u32 Result = 0;
	if(CodePoint < Info->OnePastHighestCodepoint)
	{
		Result = Font->UnicodeMap[CodePoint];
		Assert(Result < Info->GlyphCount);
	}

	return(Result);
}

internal r32
GetHorizontalAdvanceForPair(hha_font *Info, loaded_font *Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint)
{
	u32 PrevGlyph = GetGlyphFromCodePoint(Info, Font, DesiredPrevCodePoint);
	u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);

	r32 Result = Font->HorizontalAdvance[PrevGlyph*Info->GlyphCount + Glyph];

	return(Result);
}

internal r32
GetLineAdvanceFor(hha_font *Info)
{
	r32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

	return(Result);
}

internal r32
GetStartingBaselineY(hha_font *Info)
{
	r32 Result = Info->AscenderHeight;

	return(Result);
}

internal bitmap_id
GetBitmapForGlyph(game_assets *Assets, hha_font *Info, loaded_font *Font, u32 DesiredCodePoint)
{
	u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);
	bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
	Result.Value += Font->BitmapIDOffset;

	return(Result);
}

internal r32
GetLineAdvanceFor(hha_font *Info)
{
	r32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

	return(Result);
}

internal r32
font_baseline_get(struct Asset_Font *font)
{
	f32 result = font->AscenderHeight;

	return(result);
}
#endif

/*
internal r32
GetStartingBaselineY(hha_font *Info)
{
    r32 Result = Info->AscenderHeight;

    return(Result);
}
*/

u32
font_glyph_id_get(struct Asset_Font *font, u32 codepoint)
{
	struct Font_Info *info = &font->info;

	u32 result = 0;
	if(codepoint < info->codepoint_max_plus_one)
	{
		result = font->unicode_map[codepoint];
		DBG_ASSERT(result < info->glyph_count);
	}
	return(result);
}


f32
font_kerning_get(struct Asset_Font *font, u32 codepoint, u32 codepoint_previous)
{
	struct Font_Info *info = &font->info;

	u32 glyph_prev = font_glyph_id_get(font, codepoint_previous);
	u32 glyph_cur  = font_glyph_id_get(font, codepoint);

	f32 result = font->kerning_table[glyph_prev * info->glyph_count + glyph_cur];

	return(result);
}

Image
font_glyph_bitmap_get(struct Asset_Font *font, u32 codepoint)
{
	u32 id = font_glyph_id_get(font, codepoint);
	Image result = font->glyphs[id].texture.image;

	return(result);
}

struct GL_Texture
font_glyph_texture_get(struct Asset_Font *font, u32 codepoint)
{
	u32 id = font_glyph_id_get(font, codepoint);

	struct GL_Texture result = font->glyphs[id].texture;
	//result += font->bitmap_id_offset;

	return(result);
}

Vec2
font_glyph_alignment_get(struct Asset_Font *font, u32 codepoint)
{
	u32 id = font_glyph_id_get(font, codepoint);
	Vec2 result = font->glyphs[id].align_percentage;

	return(result);
}

#if 0
#define FontId u32

void
font_load(AppAssets *assets, FontId font_id)
{
	/*
	asset *Asset = Assets->Assets + ID.Value;
	Assert(Asset->State == AssetState_Unloaded);

	hha_font *Info = &Asset->HHA.Font;
	*/
	struct Asset_Font *font = &assets->font + font_id;

	/* INCOMPLETE - Square function */
	u32 kerning_table_size = sizeof(f32) * font->glyph_count * font->glyph_count;
	u32 glyphs_size = font->glyph_count * sizeof(struct Font_Glyph);
	u32 unicode_map_size = sizeof(u16) * font->codepoint_max_plus_one;
	u32 data_size = glyphs_size + kerning_table_size;
	u32 total_size = data_size + unicode_map_size;

	/*
	temporary_memory MemoryPoint = BeginTemporaryMemory(&Assets->NonRestoredMemory);
	u08 *Memory = (u08 *)PushSize(&Assets->NonRestoredMemory, SizeTotal);
	*/
	u08 memory = malloc(total_size);

	loaded_font *Font = &Asset->Font;
	font->bitmap_id_offset = GetFile(Assets, Asset->FileIndex)->AssetBase;
	font->glyphs = (struct Font_Glyph *)(memory);
	font->kerning_table = (f32 *)((u08 *)font->glyphs + glyphs_size);
	font->unicode_map = (u16 *)((u08 *)font->kerning_table + kerning_table_size);

	ZERO(font->unicode_map, unicode_map_size);

	/*
	platform_file_handle *FileHandle = GetFileHandleFor(Assets, Asset->FileIndex);
	Platform.ReadDataFromFile(FileHandle, Asset->HHA.DataOffset, SizeData, font->Glyphs);

	if(PlatformNoFileErrors(FileHandle))
	{
		hha_font *HHA = &Asset->HHA.font;
	*/
	{
		for(u32 i = 1; i < font->glyph_count; ++i)
		{
			struct Font_Glyph *glyph = font->glyphs + i;

			DBG_ASSERT(glyph->codepoint < font->codepoint_max_plus_one);
			DBG_ASSERT((u32)(u16)i == i);
			font->unicode_map[glyph->codepoint] = (u16)i;
		}

		Asset->State = AssetState_Loaded;
		KeepTemporaryMemory(MemoryPoint);
	}
	else
	{
		EndTemporaryMemory(MemoryPoint);
	}
}
#endif
