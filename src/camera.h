enum CameraProjection
{
	camera_projection_perspective,
	camera_projection_orthographic,
};

struct Camera
{
	u32 id;
	String name;

	Vec3 pos;
	Vec3 tar;
	Vec3 rot;
	Vec3 dir;

	Vec3 u; /* right */
	Vec3 v; /* up */
	Vec3 n; /* front, dir */

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

	struct Mat4_Duplex view;
	struct Mat4_Duplex projection;
	Mat4 transform;
};

void cam_load(struct Camera *cam, u32 id, String name, Vec3 pos, Vec3 tar, Vec3 rot,
	      f32 fov_h, f32 fov_v, f32 zoom, f32 nc, f32 fc,
		enum CameraProjection projection, bool locked);

void cam_load_default(struct Camera *cam, u32 id, String name);

Mat4 camera_mat_projction_backward(struct Camera *camera);
Mat4 camera_mat_projection_backward(struct Camera *camera);
