#version 330 core
layout (location = 0) out vec3 afterRefractionSecondPass;

layout(pixel_center_integer) in vec4 gl_FragCoord;

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 FragPosView;
in vec3 Normal;
in vec3 NormalInnerIntersectionPointView;
in float distanceAlongNormal;

uniform vec3 viewPos;
uniform vec2 viewportSize;

uniform mat4 viewProjection;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform sampler2D BackfaceZBuf;
uniform sampler2D BackfaceNormals;
uniform sampler2D EnvironmentMap;

uniform float indexOfRefraction;
uniform float secondIndexOfRefraction;
uniform float thetaIClampValue;

uniform bool debugBackfaceNormals;
uniform bool dTildeFix;

uniform vec3 cameraLookAtVector;

float WeigthDistance(vec3 T1, vec3 N1, vec3 V, float dV, float dN)
{
	float thetaT = acos(dot(normalize(-N1), normalize(T1)));
	float thetaI = acos(dot(normalize(V), normalize(N1)));
	thetaI = clamp(thetaI, 0.0f, thetaIClampValue);
	float thetaFraction = thetaT / thetaI;

	return (thetaFraction * dV) + ((1.0f - thetaFraction) * dN);
}

vec2 ProjectToScreenSpace(vec3 P2)
{
	vec4 P2Clip = projection * vec4(P2, 1.0f);
	vec3 P2NDC = P2Clip.xyz / P2Clip.w;

	vec2 P2Tex = P2NDC.xy;
	P2Tex.x = (P2Tex.x + 1) / 2.0f;
	P2Tex.y = (P2Tex.y + 1) / 2.0f;

	return P2Tex;

}

float DistanceFrontFaceToBackFace()
{
	float xW = gl_FragCoord.x / viewportSize.x;
	float yW = gl_FragCoord.y / viewportSize.y;
	vec2 depthTextureSampleCoords = vec2(xW, yW);
	float depthBack = texture(BackfaceZBuf, depthTextureSampleCoords).r;
	float depthFront = gl_FragCoord.z;

	float xNDC = xW * 2.0f - 1.0f;
	float yNDC = yW * 2.0f - 1.0f;
	float zNDCBack = depthBack * 2.0f - 1.0f;
	float zNDCFront = depthFront * 2.0f - 1.0f;

	vec4 backClip = vec4(xNDC, yNDC, zNDCBack, 1.0f);
	vec4 frontClip = vec4(xNDC, yNDC, zNDCFront, 1.0f);

	mat4 invProjection = inverse(projection);

	vec4 backView = invProjection * backClip;
	vec4 frontView = invProjection * frontClip;

	backView /= backView.w;
	frontView /= frontView.w;
	
	return length(backView.xyz - frontView.xyz);

}

vec3 IndexEnvironmentMap(vec3 T2)
{
	vec2 T22D = T2.xy;
	T22D.x = (T22D.x + 1) / 2.0f;
	T22D.y = (T22D.y + 1) / 2.0f;

	return texture(EnvironmentMap, T22D).rgb;
}

void main()
{
	vec3 V = normalize(FragPosView - vec3(0.0f, 0.0f, 0.0f));
	vec3 P1 = FragPosView;
	vec3 N1 = normalize(Normal);
	vec3 T1 = normalize(refract(V, N1, indexOfRefraction));
	float dV = DistanceFrontFaceToBackFace();
	float dN = distanceAlongNormal;
	float dTilde = WeigthDistance(T1, N1, V, dV, dN);
	vec3 P2 = P1 + (dTilde * T1);
	vec2 texFar = ProjectToScreenSpace(P2);
	vec3 N2;
	if(!dTildeFix)
	{
		N2 = normalize(texture(BackfaceNormals, texFar).rgb);
	}
	else
	{
		N2 = T1 - (dot(cameraLookAtVector, T1) * cameraLookAtVector);
	}
	
	vec3 T2 = normalize(refract(T1, -N2, secondIndexOfRefraction));

	if(debugBackfaceNormals)
	{
		afterRefractionSecondPass = texture(BackfaceNormals, texFar).rgb;
	}
	else
	{
		afterRefractionSecondPass = IndexEnvironmentMap(T2);
	}
}