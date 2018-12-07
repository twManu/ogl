#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;
out vec2 v_texcoord;

void main()
{
	gl_Position.xyz = a_position;
	gl_Position.w = 1;
	v_texcoord = a_texcoord;
}

