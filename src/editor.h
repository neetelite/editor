#define EOL -1
#define EOL_PTR NULL

u32 word_tokens_skip[] = {' ', '_', '.'};
u32 word_tokens_stop[] = {'_', '.'};

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
};

struct Line
{
	u32 id;
	u32 char_count;

	u32 content_count;
	u32 content_max;
	struct Content *contents;
};

struct Buffer
{
	u32 line_count;
	u32 line_max;
	struct Line *lines;
};

struct Screen
{
	/* Data about what is visible and what is not */
	i32 _ignore;
};

struct Position
{
	i64 b; /* buffer id */
	u64 y; /* line id y */
	u64 x; /* line pos x */

	i64 c; /* content id */
	i64 i; /* content index */

	bool x_min_active;
	u32 x_min;;
};

struct Range
{
	struct Position start;
	struct Position end;
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
	edit_mode_insert,
};

struct Panel
{
	struct Position pos;
	struct Screen screen;

	enum EditMode edit_mode;
};

struct Window
{
	u32 panel_id;
	u32 panel_count;
	struct Panel *panels;
};

#define EDIT_MODE_STR_SIZE 20
char edit_mode_str_table[][EDIT_MODE_STR_SIZE] =
{
	[edit_mode_normal] = "Normal",
	[edit_mode_insert] = "Insert"
};

struct Editor
{
	/* Settings */
	struct Asset_Font font;
	struct Alignment align_bar;
	struct Alignment align_buffer;
	f32 line_number_width;
	f32 margin_left;
	v4 color_background;

	u32 content_min;
	u32 content_max;

	struct Camera camera;

	u32 buffer_count;
	struct Buffer *buffers;

	u32 window_id;
	u32 window_count;
	struct Window *windows;
};

struct Editor *editor;

/* FUNCTIONS */
void cstr_draw(char *text, u32 len, v2 pos, struct Alignment *align, f32 z, v4 color);

#if 0
/* Content */
struct Content content_new(u32 id, u32 start, u32 count);
void content_data_shift_update(struct Line *line, u32 content_id);
void content_data_shift_right(struct Content *content, u32 char_pos, u32 n);
void content_char_draw(struct Content *content, u32 char_index, v4 color);
void content_draw(struct Content *content);

/* Line */
struct Line line_new(u32 id);
void line_grow(struct Line *line);
void line_content_shift_update(struct Line *line, u32 content_id);
void line_content_shift_right(struct Line *line, u32 content_id, u32 n);
void line_content_add_before(struct Line *line, u32 content_id);
void line_content_add_end(struct Line *line, u32 content_id);
void line_content_add_between(struct Line *line, u32 content_id, u32 char_pos);
void line_content_input(struct Line *line, u32 content_id, char c, u32 char_pos);
void line_input(struct Line *line, char c, u32 char_pos);
void line_number_draw(struct Line *line);
void line_draw(struct Line *line);

/* Buffer */
struct Buffer buffer_new(void);
void buffer_grow(struct Buffer *buffer);
void buffer_line_add_bellow(struct Buffer *buffer);
void buffer_cursor_move_up(struct Buffer *buffer);
void buffer_cursor_move_down(struct Buffer *buffer);
void buffer_cursor_move_right(struct Buffer *buffer);
void buffer_cursor_move_left(struct Buffer *buffer);
void buffer_cursor_move_end(struct Buffer *buffer);
void buffer_cursor_draw(struct Buffer *buffer);
void buffer_input(struct Buffer *buffer, char c);
void buffer_draw(struct Buffer *buffer);

/* Panel */
void panel_bar_draw(struct Panel *panel);
void panel_draw(struct Panel *panel);

/* Editor */
void editor_edit_mode_change(enum EditMode edit_mode);
void editor_init(void);
void editor_input(void);
void editor_update(void);
void editor_draw(void);
#endif

/* Working on */
