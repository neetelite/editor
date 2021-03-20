#ifdef OS_LINUX
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define Z_UP_RH

#define WIDTH os_context.dim.width
#define HEIGHT os_context.dim.height
#define SCREEN_DIM VEC2(WIDTH, HEIGHT)

#define FPS_MAX 60

#ifdef OS_LINUX
#include <linux/limits.h>
#endif

#include "../../library/standard.h"
#include "../../library/base.h"
#include "../../library/platform.h"
#include "../../library/graphics.h"

#define LOADER_WHITELIST
#define LOADER_LIST_OBJ

#include "include.h"

void
os_print_folder(String *path, bool recursively)
{
	/* NOTE: Folder name must end in '/' already */
	String folder_name;
	str_dup(&folder_name, path);
	cstr_bs_from_fs(folder_name.data, path->data);
	str_append_char(&folder_name, '*');

	WIN32_FIND_DATAA file_data;
	HANDLE file_handle = INVALID_HANDLE_VALUE;

	file_handle = FindFirstFile(folder_name.data, &file_data);
	if(file_handle == INVALID_HANDLE_VALUE) return;

	loop
	{
		String filename;
		str_alloc_n(&filename, PATH_MAX);
		str_cat(&filename, path, &STR(file_data.cFileName));

		u32 attributes = file_data.dwFileAttributes;
		if(attributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(!cstr_eql(file_data.cFileName, ".") &&
			   !cstr_eql(file_data.cFileName, ".."))
			{
				str_append_char(&filename, '/');
				printf("%s\n", filename.data);

				if(recursively) os_print_folder(&filename, recursively);
			}
		}
		else printf("%s\n", filename.data);
		str_free(&filename);

		bool file_was_found = FindNextFileA(file_handle, &file_data);
		if(file_was_found == false) break;
	}

	str_free(&folder_name);
}

i32
main(void)
{
	str_system_init();

	/* Path */
	os_path_build();
	//struct FileNode node = file_node_init(STR(os_state.path_run), true);
	//file_node_print(&node);
	String project_path;
	str_alloc_n(&project_path, PATH_MAX);
	str_cat(&project_path, &STR(os_state.path_src), &STR("something.project"));

	struct File project_file = file_init(project_path);
	file_open(&project_file, file_open_mode_byte_read);
	Size size = file_size(&project_file);

	String paths;
	str_alloc_n(&paths, size);
	paths.len = size;
	file_read(&project_file, paths.data, size);
	struct FileTree tree = file_tree_init(paths);
	file_tree_print(&tree);

	str_free(&project_path);
	str_free(&paths);
	file_close(&project_file);

	/* Window */
	CString window_name = "DEV | Social";
	//os_window_create(1920, 1080, window_name);
	os_window_create(1600, 900, window_name);
	//os_window_create(1080, 720, window_name);

	/* Memory */
	//os_memory_alloc(GB(4));
	os_memory_alloc(sizeof(struct App));

	/* Graphics */
	os_opengl_init();

	/* Timer */
	//struct TimeLimiter limiter = {0};
	//time_limiter_init(&limiter, TIME_NS_FROM_S(1) / FPS_MAX);

	/* Input */
	os_input_init();

	os_state.running = 1;
	while(os_state.running)
	{
		os_events();
		app_main();
		os_render();

		/* TIMER TODO */
		//time_limiter_tick(&limiter);
		//os_state.dt = limiter.dt_s;
	}

	os_memory_free();
	mem_print();
	str_system_term();
}
