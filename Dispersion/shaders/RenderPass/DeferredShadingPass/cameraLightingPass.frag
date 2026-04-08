#version 330 core
layout (location = 0) out vec3 deferredShadingPassResult;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D shadowMap;
uniform sampler2D causticMap;

struct Spotlight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float innerCutoff;
    float outerCutoff;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    //mat4 SpaceMatrix;
    //float Linear;
    //float Quadratic;
};
uniform Spotlight light;

uniform vec3 viewPos;

vec4 DispersionCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0)
    {
        return vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        return texture(causticMap, projCoords.xy); 
    }
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - pcfDepth) > 0.0001  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{         
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    float Specular = 1.0f;
    
    vec3 lighting  = Diffuse * 0.1;
    vec3 viewDir  = normalize(viewPos - FragPos);
    float distance = length(light.position - FragPos);

    vec3 lightDir = normalize(light.position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.color;

    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = light.color * spec * Specular;

    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.innerCutoff - light.outerCutoff);
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    diffuse *= intensity;
    specular *= intensity;

    float attenuation = 1.0 / (light.attenuationConstant + light.attenuationLinear * distance + light.attenuationQuadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;

    //float shadow = ShadowCalculation((light.SpaceMatrix * vec4(FragPos, 1.0)));
    //lighting += (1.0 - shadow) * (diffuse + specular);
    lighting += diffuse + specular;
    //lighting += DispersionCalculation((light.SpaceMatrix * vec4(FragPos, 1.0))).rgb;

    deferredShadingPassResult = lighting;
}