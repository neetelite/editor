#start vertex
#version 300 es

uniform mat4 u_mvp;

out vec2 v_tex;

void main()
{
	vec3 s = vec3(-0.5, -0.5, -0.5);  /* Start */
	vec3 e = vec3(0.5, 0.5, 0.5);  /* End */

	float w = 1.0;
	vec4 p0 = vec4(s.x, s.y, s.z, w);
	vec4 p1 = vec4(e.x, s.y, s.z, w);
	vec4 p2 = vec4(e.x, e.y, e.z, w);
	vec4 p3 = vec4(s.x, e.y, e.z, w);

	vec2 t0 = vec2(0, 0);
	vec2 t1 = vec2(1, 0);
	vec2 t2 = vec2(1, 1);
	vec2 t3 = vec2(0, 1);

	vec4 pos_coords[6] = vec4[6](p0, p1, p2, p0, p2, p3);
	vec2 tex_coords[6] = vec2[6](t0, t1, t2, t0, t2, t3);

	gl_Position = u_mvp * pos_coords[gl_VertexID];
	v_tex = tex_coords[gl_VertexID];
}
#end /* vertex */

#start fragment
#version 300 es
precision mediump float;

uniform vec4 u_color;
out vec4 o_frag_color;

void
main(void)
{
	o_frag_color = vec4(u_color.rgb, 1.0);
	//o_frag_color = vec4(vec3(1.0, 1.0, 1.0), 1.0);
}
#end /* fragment */
