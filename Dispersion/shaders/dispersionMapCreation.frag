#version 330 core
layout (location = 0) out vec3 dispersionMap;

in vec2 TexCoords;

uniform sampler2DArray dispersedPhotons;

void main()
{         
    int layers = textureSize(dispersedPhotons, 0).z;
    for(int layerIndex = 0; layerIndex < layers; layerIndex++)
    {
        dispersionMap += texture(dispersedPhotons, vec3(TexCoords / pow(2, layerIndex), layerIndex)).rgb;
    }
}