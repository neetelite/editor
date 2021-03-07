
struct AppMemory
{
	Size size;
	void *storage;

	bool init;
};

enum Layer
{
	layer_background,
	layer_content_background,
	layer_content_text,
	layer_cursor,
	layer_cursor_text,

	layer_count,
};

struct Canvas
{
	f32 z[layer_count];
};

struct App
{
	struct Editor editor;
	struct Canvas canvas;

	struct GL gl;
};

struct AppMemory app_memory;
struct App *app;
struct Canvas *canvas;
