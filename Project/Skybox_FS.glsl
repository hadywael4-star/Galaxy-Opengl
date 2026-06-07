#version 330 core

in vec4 frag_color;
in vec3 frag_uv;

out vec4 FragColor;

uniform samplerCube cube_map;

void main()
{
	FragColor = texture(cube_map, frag_uv);
}