#define DRAW_CONTENT_BACKGROUND 0
#define PRINT_MOVEMENT_INFO 0

/* End of  buffer */
#define EOB     -1
#define EOB_PTR NULL

/* End of line */
#define EOL     -1
#define EOL_PTR NULL

/* Tokens to stop AT */
u32 word_tokens_stop_at[] =
{
	'_', '.', ',', ';', ':',
	'!', '%', '^', '&', '=', '+', '-', '*', '/', '\\',
	'(', ')', '[', ']', '{', '}', '\'', '"'
};

struct Visual
{
	/* Visual */
	struct Rec2 rec;
};

struct Content
{
	/* Per line buffer */
	u32 id;

	u32 char_start; /* x in line */
	u32 char_count;

	/* NOTE: We have different sizes because this structure can split in two */
	u32 size;
	u32 size_alloc;
	char *data;
	struct Visual *visual; /* Allocation combined with *data */
	struct Rec2 rec;

	bool flag_draw;
	bool flag_update;
};

struct Line
{
	u32 id;
	u32 indent;

	u32 char_count;

	u32 content_count;
	u32 content_max;
	struct Content *contents;

	u32 token_count;
	u32 token_max;
	struct Token *tokens;
};

enum TokenMeaning
{
	token_type,
	token_keyword,
	token_preprocessor,

	token_define,
	token_constant,

	token_function,
	token_function_declaration,

	token_variable,
	token_variable_declaration,

	token_i8,
	token_i16,
	token_i32,
	token_i64,
	token_f32,
	token_f64,

	token_string,
	token_string_start,
	token_string_end,

	token_comment,
	token_comment_start,
	token_comment_end,

	token_brackets_start,
	token_brackets_end,

	token_braces_start,
	token_braces_end,

	token_parenthesis_start,
	token_parenthesis_end,

	token_meaning_count,
};

struct Position
{
	i64 b; /* buffer id */
	i64 y; /* line id y */
	i64 x; /* line pos x */

	i64 c; /* content id, can be EOL */
	i64 i; /* content index, can be EOL */

	bool x_min_active;
	u32 x_min;
};

struct Token
{
	u32 len;
	struct Position pos;
};

struct Buffer
{
	u32 id;

	u32 line_count;
	u32 line_max;
	struct Line *lines;

	u32 matcher_count;
	u32 matcher_max;
	struct Token *matchers;

	/* TODO: update this manually each time we add or remove a character */
	u32 char_count;
};

struct PositionPointer
{
	struct Buffer *buffer;
	struct Line *line;
	struct Content *content;
	char *c;
};

enum EditMode
{
	edit_mode_normal,
	edit_mode_command,

	edit_mode_insert,
	edit_mode_replace,
	edit_mode_visual,
	edit_mode_visual_line,

	edit_mode_count,
};

struct Range
{
	struct Position start;
	struct Position end;
};

struct Screen
{
	/* Data about the area that is visible */
	v2 pos; /* V2(0, 0) is the top left corner */
	v2 dim;

	struct Rec2 rec;
};

struct Mark
{
	struct Position pos;
};

struct Panel
{
	enum EditMode edit_mode;

	struct Position pos;
	struct Screen screen;

	struct Mark mark;
};

struct Window
{
	u32 panel_id; /* Same as buffer, differente panels per window */
	u32 panel_count;
	struct Panel *panels;
};

#define EDIT_MODE_STR_SIZE 20
char edit_mode_str_table[edit_mode_count][EDIT_MODE_STR_SIZE] =
{
	[edit_mode_normal]      = "Normal",
	[edit_mode_command]     = "Command",
	[edit_mode_insert]      = "Insert",
	[edit_mode_replace]     = "Replace",
	[edit_mode_visual]      = "Visual",
	[edit_mode_visual_line] = "Visual-Line",
};

struct Bar
{
	f32 height;
};

enum EditorMode
{
	editor_source,
	editor_build,
	editor_debug,
};

struct Appearance
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

struct Editor
{
	struct Camera camera;

	u32 buffer_count;
	struct Buffer *buffers;

	u32 window_id;
	u32 window_count;
	struct Window *windows;

	struct Bar bar;

	enum EditorMode mode;

	/* Settings */
	struct Asset_Font font;
	struct Alignment align_bar;
	struct Alignment align_buffer;

	f32 line_number_width;
	f32 margin_left;
	f32 space_size;
	u32 tab_size;

	v4 color_background;

	u32 content_min;
	u32 content_max;

	/* Debug */
	bool draw_content_background;
	bool print_movement_info;
};

struct Editor *editor;

/* FUNCTIONS */
void cstr_draw(char *text, u32 len, v2 pos, struct Alignment *align, f32 z, v4 color);
struct PositionPointer position_pointer_from_position(struct Position *position);

void position_update_next_char(struct Position *pos, struct PositionPointer *ptr);
void position_update_prev_char(struct Position *pos, struct PositionPointer *ptr);
