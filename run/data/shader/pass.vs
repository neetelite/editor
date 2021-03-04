#version 330 core

layout (location = 0) in vec3 i_pos;
layout (location = 1) in vec3 i_norm;
layout (location = 2) in vec2 i_tex;

uniform mat4 u_mat_model;
uniform mat4 u_mat_camera;

out vec3 v_pos;
out vec3 v_norm;
out vec2 v_tex;

void main()
{
	vec4 pos = u_mat_model * vec4(i_pos, 1.0);
	v_pos = pos.xyz;

	pos = u_mat_camera * pos;
	gl_Position = pos;

	v_norm = i_norm;
	v_tex = i_tex;
}
