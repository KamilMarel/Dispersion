#version 330 core
layout (location = 0) out vec3 afterLightingPass;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    afterLightingPass = texture(skybox, TexCoords).rgb;
}