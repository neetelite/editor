#define ZOOM_MUL (os_context.dim.width / 2.0)

#if 1
#define OS_WIDTH os_context.dim.width
#define OS_HEIGHT os_context.dim.height
#else
#define OS_WIDTH WIDTH
#define OS_HEIGHT HEIGHT
#endif

void
camera_projection_set(struct Camera *camera, enum CameraProjection projection)
{
	camera->projection_type = projection;

	if(camera->projection_type == camera_projection_perspective)
	{
		camera->control_pos = true;
		camera->control_rot = true;
		camera->control_zoom = false;
	}
	else if(camera->projection_type == camera_projection_orthographic)
	{
		camera->control_pos = true;
		camera->control_rot = false;
		camera->control_zoom = true;
	}
}

void
camera_dir_update(struct Camera *camera)
{
	/* Updates camera->dir based on camera->rot */
	v3 rot = v3_rad_from_deg(camera->rot);

	#if defined(Z_UP_RH)
	/* TODO(lungu): Both of these should be positive */
	f32 yaw = -rot.z;
	f32 pitch = rot.x;
	#elif defined(Y_UP_RH)
	 f32 yaw = rot.y;
	f32 pitch = rot.x;
	#endif

	f32 sin_pit = f32_sin(pitch);
	f32 sin_yaw = f32_sin(yaw);
	f32 cos_pit = f32_cos(pitch);
	f32 cos_yaw = f32_cos(yaw);

	/* TODO(lungu): Change these to right hand and left hand (not directions) */
	#if defined(Z_UP_RH)
	camera->dir.x = cos_pit * sin_yaw;
	camera->dir.y = cos_pit * cos_yaw;
	camera->dir.z = sin_pit;
	camera->n = v3_inv(camera->dir);
	#elif defined(Y_UP_RH)
	#if 0
	camera->n.x = cos_pit * sin_yaw;
	camera->n.y = -sin_pit;
	camera->n.z = cos_pit * cos_yaw;
	camera->dir = v3_inv(camera->n);
	#else
	/* LEARN NOTE(lungu): -cos_pit is needed here to lock it or something,
	   does dir.y need a a yaw too?? */
	camera->dir.x = -cos_pit*-sin_yaw;
	camera->dir.y = sin_pit;
	camera->dir.z = -cos_pit * -cos_yaw;
	camera->n = v3_inv(camera->dir);
	#endif
	#endif

	/* Get UVN */
	camera->v = V3_UP;
	camera->u = v3_cross(camera->v, camera->n);
	camera->v = v3_cross(camera->n, camera->u);

	/* Normalize */
	camera->u = v3_norm(camera->u);
	camera->v = v3_norm(camera->v);
	camera->n = v3_norm(camera->n);
}

void
camera_pos_set(struct Camera *camera, v3 pos)
{
	camera->pos = pos;
}

void
camera_tar_set(struct Camera *camera, v3 tar)
{
	camera->tar = tar;
}

void
camera_rot_set(struct Camera *camera, v3 rot)
{
	camera->rot = rot;
	camera_dir_update(camera);
}

#if 0
void
camera_dir_set(struct Camera *camera, v3 dir)
{
}
#endif

void
camera_load(struct Camera *camera, u32 id, char *name,
	    v3 pos, v3 tar, v3 rot,
	    f32 fov_h, f32 fov_v, f32 zoom,
	    f32 nc, f32 fc,
	    enum CameraProjection projection, bool locked)
{
	/* Camera 0 */
	camera->id = id;

	if(camera->name) mem_free(camera->name);
	camera->name = name;

	camera->pos = pos;
	camera->tar = tar;
	camera_pos_set(camera, pos);
	camera_tar_set(camera, tar);
	camera_rot_set(camera, rot);

	camera->fov_h = fov_h; /* NOTE(lungu): Less than 180, greater than ?? */
	camera->fov_v = fov_v; /* NOTE(lungu): Less than 180, greater than ?? */
	camera->zoom = zoom;
	camera->near_clip = nc;
	camera->far_clip = fc;
	camera_projection_set(camera, projection);

	camera->speed_pos = 5.0;
	camera->speed_rot = 6.0;
	camera->speed_zoom = 0.5;

	camera->target_locked = locked;
}

void
camera_default_load(struct Camera *camera, u32 id, char *name)
{
	camera_load(camera, id, name,
		    V3_ZERO, V3_FRONT, V3_ZERO, /* pos, tar , rot */
		    90.0f, 90.0f, 1.0, /* fov_h, fov_v, zoom, */
		    0.000001f, 2.0f,  /* nc, fc, */
		    camera_projection_perspective, true);  /* persps, locked */
}

mat4
camera_mat_pos(struct Camera *camera)
{
	mat4 result = MAT4
		(
			1, 0, 0, -camera->pos.x,
			0, 1, 0, -camera->pos.y,
			0, 0, 1, -camera->pos.z,
			0, 0, 0, 1
		);

	return(result);
}

mat4
camera_mat_rot(struct Camera *camera)
{
	mat4 result = MAT4
		(
			camera->u.x, camera->u.y, camera->u.z, 0,
			camera->v.x, camera->v.y, camera->v.z, 0,
			camera->n.x, camera->n.y, camera->n.z, 0,
			0,           0,           0,           1
		);

	return(result);
}

mat4
camera_mat_view_forward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	mat4 mat_pos = camera_mat_pos(camera);
	mat4 mat_rot = camera_mat_rot(camera);
	result = mat4_m(mat_rot, mat_pos);

	return(result);
}

mat4
//camera_mat_view_backward_gen(struct Camera *camera)
camera_mat_view_backward_gen(void)
{
	mat4 result = MAT4_ZERO;

	return(result);
}

mat4
camera_mat_view_forward(struct Camera *camera)
{
	mat4 result = camera->view.forward;
	return(result);
}

mat4
camera_mat_view_backward(struct Camera *camera)
{
	mat4 result = camera->view.backward;
	return(result);
}

mat4
camera_mat_projection_perspective_forward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	f64 fov_h = 1.0 / f32_tan(f32_rad_from_deg(camera->fov_h) / 2);
	f64 fov_v = 1.0 / f32_tan(f32_rad_from_deg(camera->fov_v) / 2);
	f32 z = camera->zoom / ZOOM_MUL;
	f32 n = camera->near_clip;
	f32 f = camera->far_clip;

	f32 r = OS_WIDTH;
	f32 t = OS_HEIGHT;
	f32 ar = r/t;

	f32 A = z*fov_h;
	f32 B = z*fov_v*ar;
	f32 C = (n+f)/(n-f);
	f32 D = (2.0*n*f)/(n-f);
	f32 E = -1.0;
	f32 F = 0.0;

	result = MAT4
		(
			A, 0, 0, 0,
			0, B, 0, 0,
			0, 0, C, D,
			0, 0, E, F
		);

	return(result);
}

mat4
camera_mat_projection_perspective_backward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	f64 fov_h = 1.0 / f32_tan(f32_rad_from_deg(camera->fov_h) / 2);
	f64 fov_v = 1.0 / f32_tan(f32_rad_from_deg(camera->fov_v) / 2);
	f32 z = camera->zoom / ZOOM_MUL;
	f32 n = camera->near_clip;
	f32 f = camera->far_clip;

	f32 r = OS_WIDTH;
	f32 t = OS_HEIGHT;
	f32 ar = r/t;

	f32 Z = (2.0*n*f)/(n-f);

	f32 A = 1.0/(z*fov_h);
	f32 B = 1.0/(z*fov_v*ar);
	f32 C = -1.0;
	f32 D = 1.0/Z;
	f32 E = ((n+f)/(n-f))/Z;

	result = MAT4
		(
			A, 0, 0, 0,
			0, B, 0, 0,
			0, 0, 0, C,
			0, 0, D, E
		);

	return(result);
}

mat4
camera_mat_projection_orthographic_forward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	f32 z = camera->zoom / ZOOM_MUL;
	f32 n = camera->near_clip;
	f32 f = camera->far_clip;

	f32 r = OS_WIDTH;
	f32 t = OS_HEIGHT;
	f32 ar = r/t;

	f32 A = z;
	f32 B = z*ar;
	f32 C = 2.0/(n-f);
	f32 D = (n+f)/(n-f);
	f32 E = 0.0;
	f32 F = 1.0;

	result = MAT4
		(
			A, 0, 0, 0,
			0, B, 0, 0,
			0, 0, C, D,
			0, 0, E, F
		);

	return(result);
}

mat4
camera_mat_projection_orthographic_backward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	f32 z = camera->zoom / ZOOM_MUL;
	f32 n = camera->near_clip;
	f32 f = camera->far_clip;

	f32 r = OS_WIDTH;
	f32 t = OS_HEIGHT;
	f32 ar = r/t;

	f32 Z = 2.0/(n-f);
	f32 A = 1.0/(z);
	f32 B = 1.0/(z*ar);
	f32 C = 1.0/(Z);
	f32 D = -((n+f)/(n-f))/(Z);
	f32 E = 1.0;

	result = MAT4
		(
			A, 0, 0, 0,
			0, B, 0, 0,
			0, 0, C, D,
			0, 0, 0, E
		);

	return(result);
}

mat4
camera_mat_projection_forward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	if(camera->projection_type == camera_projection_perspective)
	{
		result = camera_mat_projection_perspective_forward_gen(camera);
	}
	else if(camera->projection_type == camera_projection_orthographic)
	{
		result = camera_mat_projection_orthographic_forward_gen(camera);
	}

	return(result);
}

mat4
camera_mat_projection_backward_gen(struct Camera *camera)
{
	mat4 result = MAT4_ZERO;

	if(camera->projection_type == camera_projection_perspective)
	{
		result = camera_mat_projection_perspective_backward_gen(camera);
	}
	else if(camera->projection_type == camera_projection_orthographic)
	{
		result = camera_mat_projection_orthographic_backward_gen(camera);
	}

	return(result);
}

mat4
camera_mat_projection_forward(struct Camera *camera)
{
	mat4 result = camera->projection.forward;
	return(result);
}

mat4
camera_mat_projection_backward(struct Camera *camera)
{
	mat4 result = camera->projection.backward;
	return(result);
}

void
camera_matrix_update(struct Camera *camera)
{
	camera_dir_update(camera);

	camera->view.forward = camera_mat_view_forward_gen(camera);
	//camera->view.backward = camera_mat_view_backward_gen(camera);
	camera->view.backward = camera_mat_view_backward_gen();

	camera->projection.forward = camera_mat_projection_forward_gen(camera);
	camera->projection.backward = camera_mat_projection_backward_gen(camera);

	camera->transform = mat4_m(camera->projection.forward, camera->view.forward);
}

void
camera_update(struct Camera *camera)
{
	camera_matrix_update(camera);
}
