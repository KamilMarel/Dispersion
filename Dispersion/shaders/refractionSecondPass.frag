#version 330 core
layout (location = 0) out vec3 rNormalFront;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

void main()
{
	rNormalFront = normalize(Normal);
}