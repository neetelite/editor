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
	Char *data;
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

struct Buffer
{
	u32 id;

	u32 line_count;
	u32 line_max;
	struct Line *lines;

	u32 matcher_count;
	u32 matcher_max;
	struct Token *matchers;

	enum LanguageType language;

	/* TODO: update this manually each time we add or remove a character */
	u32 char_count;
};

struct PositionPointer
{
	struct Buffer *buffer;
	struct Line *line;
	struct Content *content;
	Char *c;
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
	Vec2 pos; /* V2(0, 0) is the top left corner */
	Vec2 dim;

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
Char edit_mode_str_table[edit_mode_count][EDIT_MODE_STR_SIZE] =
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

	Vec4 color_background;

	u32 content_min;
	u32 content_max;

	/* Debug */
	bool draw_content_background;
	bool print_movement_info;
};

struct Editor *editor;

/* FUNCTIONS */
void cstr_draw(CString text, u32 len, Vec2 pos, struct Alignment *align, f32 z, Vec4 color);
struct PositionPointer position_pointer_from_position(struct Position *position);

void position_update_next_char(struct Position *pos, struct PositionPointer *ptr);
void position_update_prev_char(struct Position *pos, struct PositionPointer *ptr);
