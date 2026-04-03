#version 330 core
layout (location = 0) out vec3 blurredImage;

in vec2 TexCoords;

uniform sampler2D imageToBlur;
uniform int viewportSize;

uniform float coefs[1000];
uniform int kernelSize;

void main()
{
    //blurredImage = texture(imageToBlur, TexCoords).rgb;
    
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    vec2 uv;
    int coefIndex = 0;
    int offset = kernelSize / 2;
    for(int y = offset; y >= -offset; y--)
    {
        for(int x = -offset; x <= offset; x++)
        {
            uv = (gl_FragCoord.xy + vec2(x, y)) / viewportSize;
            color += texture(imageToBlur, uv).rgb * coefs[coefIndex];
            coefIndex++;
        }
    }


    blurredImage = color;
    
}