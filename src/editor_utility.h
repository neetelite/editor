/* Content */
u32 content_char_index_from_pos(struct Content *content, u32 char_pos);
u32 content_char_end(struct Content *content);
u32 content_size_end(struct Content *content);
bool content_char_index_is_end(struct Content *content, u32 char_index);
bool content_char_pos_is_end(struct Content *content, u32 char_pos);
bool content_is_empty(struct Content *content);
bool content_is_full(struct Content *content);

/* Line */
//char *line_contet_char_get_next(struct Line *line, u32 content_id);
struct Content *line_content_get_by_id(struct Line *line, i32 content_id);
struct Content *line_content_get_by_char_pos(struct Line *line, u32 char_pos);
struct Content *line_content_get_first(struct Line *line);
struct Content *line_content_get_last(struct Line *line);
struct Content * line_content_get_previous(struct Line *line, struct Content *content);
struct Content * line_content_get_next(struct Line *line, struct Content *content);
bool line_content_is_first(struct Line *line, struct Content *content);
bool line_content_is_last(struct Line *line, struct Content *content);
bool line_is_empty(struct Line *line);
bool line_is_full(struct Line *line);

/* Panel */
struct Content *position_content_get_previous(struct Panel *panel);
struct Content *position_content_get_next(struct Panel *panel);
struct Line *position_line_get_previous(struct Panel *panel);
struct Line *position_line_get_next(struct Panel *panel);

/* Buffer */
struct Line *buffer_line_get_by_id(struct Buffer *buffer, i32 line_id);

/* Editor */
struct Buffer *editor_buffer_get_by_id(i32 buffer_id);
struct Panel *editor_panel_get_by_id(i32 panel_id);
