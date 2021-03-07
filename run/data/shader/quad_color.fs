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
