#version 330 core

uniform sampler2D s;

in vec2 v_tex;
out vec4 o_frag_color;

void
main(void)
{
	vec4 c = texture(s, v_tex);
	//vec4 c = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);

	o_frag_color = c;
}
