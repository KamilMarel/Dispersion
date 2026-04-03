#version 330 core

layout (location = 0) in vec4 aPos;

uniform sampler2D photonBuffer;
uniform mat4 viewProj;

out vec3 NDCPos;

void main()
{
	vec2 texPos = aPos.xy;
	texPos.x = (texPos.x + 1) / 2.0f;
	texPos.y = (texPos.y + 1) / 2.0f;

    vec4 refractedPos = viewProj * vec4(texture(photonBuffer, texPos).rgb, 1.0f);
	refractedPos /= refractedPos.w;
	
	NDCPos = refractedPos.xyz;

	gl_Position = vec4(aPos.xyz, 1.0f);
}