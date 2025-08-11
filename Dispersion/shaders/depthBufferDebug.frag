#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gDepth;

void main()
{             
    float FragPos = texture(gDepth, TexCoords).r;
    FragColor = vec4(vec3(FragPos), 1.0);
}