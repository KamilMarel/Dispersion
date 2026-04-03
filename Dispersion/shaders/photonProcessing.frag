#version 330 core
//layout(pixel_center_integer) in vec4 gl_FragCoord;

layout (location = 0) out vec4 photon;

uniform vec2 viewportSize;

in vec2 photonPosition;

void main()
{   
	photon = vec4(photonPosition, 0.0f, 1.0f);
}