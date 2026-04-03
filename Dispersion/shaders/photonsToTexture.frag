#version 330 core

layout (location = 0) out vec3 photonsTexture;

in vec3 NDCPos;

void main()
{    
    photonsTexture = NDCPos;
}