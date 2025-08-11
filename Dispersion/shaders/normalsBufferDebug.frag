#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gNormal;

void main()
{             
    vec3 FragPos = texture(gNormal, TexCoords).rgb;
    FragColor = vec4(FragPos, 1.0);
}