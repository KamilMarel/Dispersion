#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D rNormalBack;

void main()
{             
    vec3 FragPos = texture(rNormalBack, TexCoords).rgb;
    FragColor = vec4(FragPos, 1.0);
}