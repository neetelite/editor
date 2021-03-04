#version 330
 
varying vec2 v_tex;

void main()
{
	float n = -1.0;
	float p = 1.0;
	float z = 0.5;
	float w = 1.0;

	vec4 v0 = vec4(n, n, z, w);
	vec4 v1 = vec4(p, n, z, w);
	vec4 v2 = vec4(p, p, z, w);   
	vec4 v3 = vec4(n, p, z, w);   

	vec2 t0 = vec2(0, 0);
	vec2 t1 = vec2(1, 0);
	vec2 t2 = vec2(1, 1);
	vec2 t3 = vec2(0, 1);

	vec4 vertices[6] = vec4[6](v0, v1, v2, v0, v2, v3);
	vec2 tex_coords[6] = vec2[6](t0, t1, t2, t0, t2, t3);

	gl_Position = vertices[gl_VertexID];  
	v_tex = tex_coords[gl_VertexID];
}