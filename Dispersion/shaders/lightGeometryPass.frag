#version 330 core
layout (location = 0) out vec3 gPosition;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

void main()
{    
    gPosition = FragPos;
}