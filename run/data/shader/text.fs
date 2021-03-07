#version 300 es
precision mediump float;

in vec2 v_tex;
out vec4 o_color_frag;

uniform sampler2D bitmap;
uniform vec4 u_color;

void main()
{
    float alpha = texture(bitmap, v_tex).a;
    o_color_frag  = vec4(u_color.xyz, alpha);
}
