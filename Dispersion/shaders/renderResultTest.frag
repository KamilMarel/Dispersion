#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D renderResult;
uniform sampler2DArray arrayRenderResult;
uniform bool depthDebug;
uniform bool arrayTextureDebug;
uniform float arrayTextureLayer;

float near = 0.1; 
float far  = 10.0; 
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{   
    if(!arrayTextureDebug)
    {
        if(!depthDebug)
        {
            FragColor = vec4(texture(renderResult, TexCoords).rgb, 1.0f);
        }
        else
        {
            float FragPos = LinearizeDepth(texture(renderResult, TexCoords).r) / far;
            FragColor = vec4(vec3(FragPos), 1.0);
        }
    }
    else
    {
        FragColor = vec4(texture(arrayRenderResult, vec3(TexCoords, arrayTextureLayer)).rgb, 1.0f);
    }
}