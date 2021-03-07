#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define Z_UP_RH

#define WIDTH os_context.dim.width
#define HEIGHT os_context.dim.height
#define SCREEN_DIM V2(WIDTH, HEIGHT)

#define FPS_MAX 60

#include <linux/limits.h>

#include "../../lib/standard.h"
#include "../../lib/base.h"
#include "../../lib/platform.h"
#include "../../lib/graphics.h"

#define LOADER_WHITELIST
#define LOADER_LIST_OBJ

#include "include.h"

i32
main(void)
{
	/* Path */
	os_path_build();

	/* Window */
	char *window_name = "DEV | Social";
	//os_window_create(1920, 1080, window_name);
	os_window_create(1600, 900, window_name);
	//os_window_create(1080, 720, window_name);

	/* Memory */
	//os_memory_alloc(GB(4));
	os_memory_alloc(sizeof(struct App));

	/* Graphics */
	os_opengl_init();

	/* Timer */
	struct TimeLimiter limiter = {0};
	time_limiter_init(&limiter, TIME_NS_FROM_S(1) / FPS_MAX);

	/* Input */
	os_input_init();

	os_state.running = 1;
	while(os_state.running)
	{
		os_events();
		app_main();
		os_render();

		/* TIMER TODO */
		time_limiter_tick(&limiter);
		os_state.dt = limiter.dt_s;
	}

	os_memory_free();
	mem_print();
}
