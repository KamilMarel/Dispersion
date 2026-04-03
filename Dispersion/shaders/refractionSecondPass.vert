#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in vec3 aNormalInnerIntersectionPoint;
layout (location = 8) in float aDistanceAlongNormal;

out vec3 FragPos;
out vec3 FragPosView;
out vec2 TexCoords;
out vec3 Normal;
out vec3 NormalInnerIntersectionPointView;
out float distanceAlongNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    FragPosView = (view * worldPos).xyz;
    TexCoords = aTexCoords;
    distanceAlongNormal = aDistanceAlongNormal;
    NormalInnerIntersectionPointView = (view * model * vec4(aNormalInnerIntersectionPoint, 0.0f)).xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;
}