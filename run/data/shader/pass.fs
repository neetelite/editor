#version 300 es

#define LIGHTING 1
#define POINT_LIGHT 0


#define i32 int
#define f32 float
#define f64 double
#define v2 vec2
#define V2 vec2
#define v3 vec3
#define V3 vec3
#define v4 vec4
#define V4 vec4

uniform v4 u_color;
uniform v3 u_camera_pos;

in v3 v_pos;
in v3 v_norm;
in v2 v_tex;

out v4 o_color_frag;

void main()
{
	#if LIGHTING
	/* Object */
	v3 object_color = u_color.rgb;
	v3 object_normal = v_norm;

	/* Camera */
	v3 camera_pos = u_camera_pos;
	v3 camera_dir = normalize(camera_pos - v_pos.xyz);

	/* Light */
	f32 light_strength = 0.8;
	v3 light_color = V3(0.9, 0.9, 1.0) * light_strength;
	v3 light_pos = V3(-1.0, 2.0, 5.0);

	#if POINT_LIGHT
	v3 light_dir = normalize(light_pos - v_pos.xyz); /* Point */
	#else
	v3 light_dir = normalize(-V3(-0.2, 0.2, -0.4));
	#endif
	v3 reflection_dir = reflect(-light_dir, object_normal);

	/* Ambient */
	f32 light_ambient_strength = 0.2;
	v3 light_ambient = light_color * light_ambient_strength;

	/* Diffuse */
	f32 diffuse = max(dot(object_normal, light_dir), 0.0);
	v3 light_diffuse = diffuse * light_color;

	/* Specular */
	f32 light_specular_strength = 0.5;
	f32 spec = pow(max(dot(camera_dir, reflection_dir), 0.0), 128);
	v3 light_specular = light_specular_strength * spec * light_color;

	v3 light = light_ambient + light_diffuse + light_specular;
	v4 color = V4(light * object_color, 1.0);
	//v4 color = V4(abs(object_color), 1.0);

	o_color_frag = color;
	#else
	o_color_frag = u_color;
	#endif
}
