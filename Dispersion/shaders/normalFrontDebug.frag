#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D rNormalFront;

void main()
{             
    vec3 FragPos = texture(rNormalFront, TexCoords).rgb;
    FragColor = vec4(FragPos, 1.0);
}