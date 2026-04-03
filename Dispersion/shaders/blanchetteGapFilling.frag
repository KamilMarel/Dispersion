#version 330 core
layout (location = 0) out vec3 dispersionMapWithoutGaps;

in vec2 TexCoords;

uniform sampler2D dipsersionMapWithGaps;

uniform vec2 viewportSize;
uniform float fetchRadius;
uniform int samplesCount;
uniform int hitCountThreshold;
uniform float colorThreshold;

float randomOffset = 0.0f;
float random (vec2 st) {
    randomOffset++;
    return fract(sin(dot(st.xy + vec2(randomOffset, 0.0f),
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void main()
{
    vec2 initPos = gl_FragCoord.xy;

    float xSign, ySign;

    vec3 originalColor = texture(dipsersionMapWithGaps, TexCoords).rgb;
    vec3 newColor = vec3(0.0f);
    int hitCounter = 0;
    for(int sampleIndex = 0; sampleIndex < samplesCount; sampleIndex++)
    {
        if(random(initPos) > 0.5f)
        {
            xSign = 1.0f;
        }
        else
        {
            xSign = -1.0f;
        }
        if(random(initPos) > 0.5f)
        {
            ySign = 1.0f;
        }
        else
        {
            ySign = -1.0f;
        }
        vec2 randomUnitVector = vec2(1.0f * xSign * random(initPos), 
                                     1.0f * ySign * random(initPos));
        randomUnitVector = normalize(randomUnitVector);
        vec2 randomPos = initPos + (randomUnitVector * fetchRadius);
        vec2 randomTexPos = randomPos / viewportSize;
        vec3 fetchedColor = texture(dipsersionMapWithGaps, randomTexPos).rgb;
        if(fetchedColor.rgb != vec3(0.0f))
        {
            newColor += fetchedColor;
            hitCounter++;
        }
    }
    newColor /= samplesCount;
    if(hitCounter > hitCountThreshold)
    {
        dispersionMapWithoutGaps = newColor;
    }
    else
    {   
        dispersionMapWithoutGaps = originalColor;
    }
}