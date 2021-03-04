/* Globals */
void
app_init(void)
{
	app = os_state.app_memory;

	gl_init();
	editor_init();

	canvas = &app->canvas;
	f32 layer_increment = (1.0 / (layer_count-1));
	canvas->z[0] = -1.0;
	for(i32 i = 1; i < layer_count; ++i)
	{
		canvas->z[i] = canvas->z[i-1] + layer_increment;
	}
	canvas->z[0] += 0.001;
	canvas->z[layer_count-1] -= 0.001;

	app_memory.init = true;
}

void
app_term(void)
{

}

void
app_frame(void)
{
	gl_viewport_depth_clear();
	editor_update();
	editor_draw();
}

void
app_main(void)
{
	if(app_memory.init == false) app_init();
	app_frame();
	if(os_state.running == false) app_term();
}
