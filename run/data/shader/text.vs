#version 330 core

layout (location = 0) in vec4 i_pos;
out vec2 v_tex;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(i_pos.xy, 0.9, 1.0);
    v_tex = i_pos.zw;
}
