#version 330 core

#define M_PI 3.1415927

/* Uniforms */
uniform float u_snap;
uniform float u_zoom;
uniform vec4 u_box;
uniform vec4 u_color_major;
uniform vec4 u_color_minor;
uniform vec4 u_color_background;
uniform mat4 u_mvp_grid;
uniform vec2 u_pos;

uniform vec2 u_resolution;
uniform vec2 u_center;

in vec2 v_tex;

out vec4 o_color_frag;

/* Globals */
float g_antialias = 1.0;

/* Cartesian and projected limits as xmin, xmax, ymin, ymax */
//vec4 g_limits_1 = vec4(-1.0, +1.0, -1.0, +1.0);
//vec4 g_limits_2 = vec4(-1.0, +1.0, -1.0, +1.0);

vec4 g_limits_1 = vec4(-1.0, +1.0, -1.0, +1.0);
vec4 g_limits_2 = vec4(-1.0, +1.0, -1.0, +1.0);

vec2 g_major_grid_step = vec2(1.0, 1.0);
vec2 g_minor_grid_step = vec2(0.1, 0.1);

float g_major_grid_width = 1.0;
float g_minor_grid_width = 1.0;

/* Forward Transform (polar) */
vec2
transform_forward(vec2 pos)
{
	vec2 result;
	result = pos;
	return(result);
}

vec2
transform_inverse(vec2 pos)
{
	vec2 result;
	result = pos;
	return(result);
}

vec2
scale_forward(vec2 pos, vec4 limits)
{
	vec2 result = pos;
	result += vec2(0.5, 0.5);
	result *= vec2(limits[1] - limits[0], limits[3] - limits[2]);
	result += vec2(limits[0], limits[2]);
	return(result);
}

vec2
scale_inverse(vec2 pos, vec4 limits)
{
	vec2 result = pos;
	result -= vec2(limits[0], limits[2]);
	result /= vec2(limits[1] - limits[0], limits[3] - limits[2]);
	result -= vec2(0.5, 0.5);
	return(result);
}

float
stroke_alpha(float distance, float line_width, float antialias)
{
	/* Antialias strokes alpha coefficient */

	float result = 0.0;

	float t = (line_width/2.0) - antialias;

	float signed_distance = distance;
	float border_distance = abs(signed_distance) - t;

	float alpha = border_distance/antialias;
	alpha = exp(-alpha*alpha);

	if(border_distance > line_width/2.0 + antialias) result = 0.0;
	else if(border_distance < 0.0) result = 1.0;
	else result = alpha;

	return(result);
}

float
get_tick(float t, float v_min, float v_max, float step)
{
	float result = 0.0;

	float first_tick = floor((v_min + step/2.0)/step) * step;
	float last_tick = floor((v_max + step/2.0)/step) *step;
	float tick = v_min + t*(v_max-v_min);

	if(tick < (v_min + (first_tick-v_min)/2.0)) result = v_min;
	if(tick > (last_tick + (v_max-last_tick)/2.0)) result = v_max;

	tick += step/2.0;
	tick = floor(tick/step) * step;

	result = min(max(v_min, tick), v_max);

	return(result);
}

float
screen_distance(vec4 a, vec4 b)
{
	/* Compute the distance (in screen coordinates) between A and B */

	float result = 0.0;

	vec4 pos_a = u_mvp_grid * a;
	pos_a /= pos_a.w;
	pos_a.xy = pos_a.xy * u_resolution.xy/2.0;

	vec4 pos_b = u_mvp_grid * b;
	pos_b /= pos_b.w;
	pos_b.xy = pos_b.xy * u_resolution.xy/2.0;

	result = length(pos_a.xy - pos_b.xy);

	return(result);
}


void
main()
{
	vec2 g_major_grid_step = vec2(u_snap, u_snap);
	vec2 g_minor_grid_step = g_major_grid_step * 0.1;

	float a = 1.0;
	g_limits_1 = vec4(-a, a, -a, a);
	g_limits_2 = vec4(-a, a, -a, a);

	/* Mouse */
	//vec2 pos = u_pos;
	vec2 pos = u_pos + u_center;

	#if 1
	/* Scale and trans */
	g_limits_1 /= u_zoom;
	vec4 g_limits_1_pos = g_limits_1;
	g_limits_1_pos.xy -= vec2(pos.x, pos.x);
	g_limits_1_pos.zw -= vec2(pos.y, pos.y);
	g_limits_1 = g_limits_1_pos;
	#endif

	//g_limits_1.xy -= 1;
	//g_limits_1.wz -= 1;

	/* Limits 2 */
	g_limits_2 = g_limits_1;
	//g_limits_2.xy -= 1;
	//g_limits_2.wz = g_limits_1.wz;

	/* Aspect correction */
	float aspect_ratio = u_resolution.y/u_resolution.x;
	g_limits_1.wz *= aspect_ratio;
	g_limits_2.wz *= aspect_ratio;

	vec2 NP1 = v_tex;
	/* Change from 0/1 to -0.5/+0.5 */
	NP1 -= 0.5;

	vec2 P1 = scale_forward(NP1, g_limits_1);
	vec2 P2 = transform_inverse(P1);

	/* Test if we are withing limits but we do not discard the
	fragment yet because we want to draw border. Discarding
	would mean the the exterior would not be drawn */
	bvec2 outside = bvec2(false);
	if(P2.x < g_limits_2[0]) outside.x = true;
	if(P2.x > g_limits_2[1]) outside.x = true;
	if(P2.y < g_limits_2[2]) outside.y = true;
	if(P2.y > g_limits_2[3]) outside.y = true;

	vec2 NP2 = scale_inverse(P2, g_limits_2);
	vec2 P;
	float tick;

	/* Major tick, X axis */
	tick = get_tick(NP2.x+0.5, g_limits_2[0], g_limits_2[1], g_major_grid_step.x);
	P = transform_forward(vec2(tick, P2.y));
	P = scale_inverse(P, g_limits_1);
	float Mx = screen_distance(vec4(NP1, 0, 1), vec4(P, 0, 1));

	/* Minor tick, X axis */
	tick = get_tick(NP2.x+0.5, g_limits_2[0], g_limits_2[1], g_minor_grid_step.x);
	P = transform_forward(vec2(tick, P2.y));
	P = scale_inverse(P, g_limits_1);
	float mx = screen_distance(vec4(NP1, 0, 1), vec4(P, 0, 1));

	/* Major tick, Y axis */
	tick = get_tick(NP2.y+0.5, g_limits_2[2], g_limits_2[3], g_major_grid_step.y);
	P = transform_forward(vec2(P2.x, tick));
	P = scale_inverse(P, g_limits_1);
	float My = screen_distance(vec4(NP1, 0, 1), vec4(P, 0, 1));

	/* Minor tick, Y axis */
	tick = get_tick(NP2.y+0.5, g_limits_2[2], g_limits_2[3], g_minor_grid_step.y);
	P = transform_forward(vec2(P2.x, tick));
	P = scale_inverse(P, g_limits_1);
	float my = screen_distance(vec4(NP1, 0, 1), vec4(P, 0, 1));

	float M = min(Mx, My);
	float m = min(mx, my);

	/* Here we take care of *finishing* the border lines */
	if(outside.x && outside.y)
	{
		if(Mx > 0.5*(g_major_grid_width + g_antialias)) discard;
		else if(My > 0.5*(g_major_grid_width + g_antialias)) discard;
		else M = max(Mx, My);
	}
	else if(outside.x)
	{
		if(Mx > 0.5*(g_major_grid_width + g_antialias)) discard;
		else M = m = Mx;
	}
	else if(outside.y)
	{
		if(My > 0.5*(g_major_grid_width + g_antialias)) discard;
		else M = m = My;
	}

	/* Mix major/minor colors to get dominant color */
	vec4 color = u_color_major;
	float alpha1 = stroke_alpha(M, g_major_grid_width, g_antialias);
	float alpha2 = stroke_alpha(m, g_minor_grid_width, g_antialias);

	float alpha = alpha1;
	if(alpha2 > alpha1*1.5)
	{
		alpha = alpha2;
		color = u_color_minor;
	}

	/* Without extra cost, we can also project a texture */
	#if 0
	vec4 tex_color = texture2D(u_texture, vec2(NP2.x, 1.0-NP2.y));
	o_color_frag = mix(tex_color, color, alpha);
	#endif


	//o_color_frag = mix(u_color_background, color, alpha);
	//o_color_frag = vec4(mix(u_color_background, color, alpha).rgb, 1.0);
	o_color_frag = vec4(color.rgb, color.a*alpha);
}
