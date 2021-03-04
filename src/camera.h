enum CameraProjection
{
	camera_projection_perspective,
	camera_projection_orthographic,
};

struct Camera
{
	u32 id;
	char *name;

	v3 pos;
	v3 tar;
	v3 rot;
	v3 dir;

	v3 u; /* right */
	v3 v; /* up */
	v3 n; /* front, dir */

	f32 fov_h;
	f32 fov_v;
	f32 zoom;
	f32 near_clip;
	f32 far_clip;

	enum CameraProjection projection_type;

	/* Control */
	bool control_pos;
	bool control_rot;
	bool control_zoom;

	f32 speed_pos;
	f32 speed_rot;
	f32 speed_zoom;

	/* Lock */
	bool target_locked;

	struct mat4_Duplex view;
	struct mat4_Duplex projection;
	mat4 transform;
};

void cam_load(struct Camera *cam, u32 id, char *name, v3 pos, v3 tar, v3 rot,
	      f32 fov_h, f32 fov_v, f32 zoom, f32 nc, f32 fc,
		enum CameraProjection projection, bool locked);

void cam_load_default(struct Camera *cam, u32 id, char *name);

mat4 camera_mat_projction_backward(struct Camera *camera);
mat4 camera_mat_projection_backward(struct Camera *camera);
