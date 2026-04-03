#version 330 core
layout (points) in;
layout (points, max_vertices = 7) out;

uniform mat4 viewProj;
uniform mat4 view;
uniform vec2 viewportSize;
uniform float solidAngle;
uniform float customSolidAngle;

uniform bool ccdOptimizationEnabled;
uniform bool photonCCDBasedCalculationEnabled;
uniform bool blanchetteGapFillingOverride;
uniform bool useCustomSolidAngle;
uniform bool ignoreDispersion;

uniform sampler2D redPhotonPositions;
uniform sampler2D greenPhotonPositions;
uniform sampler2D bluePhotonPositions;
uniform sampler2D orangePhotonPositions;
uniform sampler2D yellowPhotonPositions;
uniform sampler2D indigoPhotonPositions;
uniform sampler2D purplePhotonPositions;

//uniform sampler2D photonsTexture;

out vec3 color;
flat out int splatSize;
out vec2 centerPosScreen;

vec3 colors[7] = vec3[](vec3(1.0f, 0.0f, 0.0f),
						vec3(1.0f, 0.3f, 0.0f),
						vec3(1.0f, 1.0f, 0.0f),
						vec3(0.37f, 1.0f, 0.0f),
						vec3(0.0f, 0.84f, 1.0f),
						vec3(0.24f, 0.0f, 1.0f),
						vec3(0.38f, 0.0f, 0.38f));

int getCorrectSplatSize(float splatSize)
{
	if(splatSize >= 15.0f)
	{
		return 15;
	}
	if(splatSize <= 3.0f)
	{
		return 3;
	}

	int result = int(round(splatSize));

	while(result != 15 &&
		  result != 11 &&
		  result != 7 &&
		  result != 3)
	{
		result++;
	}

	return result;
}

vec2 getScreenPosition(vec2 NDCPosition)
{
	vec2 texPos = NDCPosition;
	texPos.x = (texPos.x + 1) / 2.0f;
	texPos.y = (texPos.y + 1) / 2.0f;

	vec2 screenPos = texPos;
	screenPos.x = screenPos.x * viewportSize.x;
	screenPos.y = screenPos.y * viewportSize.y;

	return screenPos;
}

float calculatePhotonQuadrilateralArea(vec2 vertices[4])
{
	float x1 = vertices[0].x;
	float y1 = vertices[0].y;
	float x2 = vertices[1].x;
	float y2 = vertices[1].y;
	float x3 = vertices[2].x;
	float y3 = vertices[2].y;
	float x4 = vertices[3].x;
	float y4 = vertices[3].y;

	return ( 0.5f * ( (x1*y2 + x2*y3 + x3*y4 + x4*y1) - (x2*y1 + x3*y2 + x4*y3 + x1*y4) ) );
}

bool checkQuadrilateral(vec2 vertices[4])
{
	vec2 a = vertices[0];
	vec2 b = vertices[1];
	vec2 c = vertices[2];

	if((b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x))
	{
		return false;
	}

	a = vertices[0];
	b = vertices[1];
	c = vertices[3];

	if((b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x))
	{
		return false;
	}

	a = vertices[1];
	b = vertices[2];
	c = vertices[3];

	if((b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x))
	{
		return false;
	}

	a = vertices[0];
	b = vertices[2];
	c = vertices[3];

	if((b.x - a.x) * (c.y - a.y) == (b.y - a.y) * (c.x - a.x))
	{
		return false;
	}

	return true;
}

vec2 getTexPosition(vec2 NDCPosition)
{
	vec2 texPos = NDCPosition;
	texPos.x = (texPos.x + 1.0f) / 2.0f;
	texPos.y = (texPos.y + 1.0f) / 2.0f;
	return texPos;
}

float calculateMaxDistanceForAdjacentPhotons(vec2 photonsNDC[4])
{
	float result = 0.0f;

	for(int firstPhotonIndex = 0; 
			firstPhotonIndex < 4; 
			firstPhotonIndex++)
	{
		for(int secondPhotonIndex = firstPhotonIndex + 1; 
				secondPhotonIndex < 4; 
				secondPhotonIndex++)
		{
			float dist = distance(photonsNDC[firstPhotonIndex],
								  photonsNDC[secondPhotonIndex]);
			if(dist > result)
			{
				result = dist;
			}
		}
	}

	return result;
}

bool adjacentPhotonsFound = false;
vec3[4] getRefractedAdjacentPhotonsWorldPos(vec2 basePhotonTexPos)
{
	vec3 result[4];

	float texStep = 1.0f / viewportSize.x;

	result[0] = texture(purplePhotonPositions, basePhotonTexPos).rgb;

	if(texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, 0.0f)).rgb != vec3(1.0f, 1.0f, 1.0f))
	{
		result[1] = texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, 0.0f)).rgb;

		if(texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, -texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
		{
			result[2] = texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, -texStep)).rgb;

			if(texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, -texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
			{
				result[3] = texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, -texStep)).rgb;
				adjacentPhotonsFound = true;
				return result;
			}
		}
		else if(texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
		{
			result[2] = texture(purplePhotonPositions, basePhotonTexPos + vec2(-texStep, texStep)).rgb;

			if(texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
			{
				result[3] = texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, texStep)).rgb;
				adjacentPhotonsFound = true;
				return result;
			}
		}
	}
	else if(texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, 0.0f)).rgb != vec3(1.0f, 1.0f, 1.0f))
	{
		result[1] = texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, 0.0f)).rgb;

		if(texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, -texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
		{
			result[2] = texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, -texStep)).rgb;

			if(texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, -texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
			{
				result[3] = texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, -texStep)).rgb;
				adjacentPhotonsFound = true;
				return result;
			}
		}
		else if(texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
		{
			result[2] = texture(purplePhotonPositions, basePhotonTexPos + vec2(texStep, texStep)).rgb;

			if(texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, texStep)).rgb != vec3(1.0f, 1.0f, 1.0f))
			{
				result[3] = texture(purplePhotonPositions, basePhotonTexPos + vec2(0.0f, texStep)).rgb;
				adjacentPhotonsFound = true;
				return result;
			}
		}
	}

	return result;
}

void main() 
{
	vec2 texPos = gl_in[0].gl_Position.xy;
	texPos.x = (texPos.x + 1.0f) / 2.0f;
	texPos.y = (texPos.y + 1.0f) / 2.0f;

	vec4 NDCPos[7];
	for(int i = 0; i < 7; i++)
	{
		switch(i)
		{
			case 0:
			{
				NDCPos[i] = viewProj * vec4(texture(redPhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 1:
			{
				NDCPos[i] = viewProj * vec4(texture(orangePhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 2:
			{
				NDCPos[i] = viewProj * vec4(texture(yellowPhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 3:
			{
				NDCPos[i] = viewProj * vec4(texture(greenPhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 4:
			{
				NDCPos[i] = viewProj * vec4(texture(bluePhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 5:
			{
				NDCPos[i] = viewProj * vec4(texture(indigoPhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			case 6:
			{
				NDCPos[i] = viewProj * vec4(texture(purplePhotonPositions, texPos).rgb, 1.0f);
				NDCPos[i] /= NDCPos[i].w;
				break;
			}
			default:
			{
				break;
			}
		}
	}

	bool optimizationSkip[7];
	if(ccdOptimizationEnabled)
	{
		for(int i = 0; i < 7; i++)
		{
			vec2 screenFinalPosition = getScreenPosition(NDCPos[i].xy);
			screenFinalPosition.x = int(screenFinalPosition.x);
			screenFinalPosition.y = int(screenFinalPosition.y);
			float xRemainder = mod(screenFinalPosition.x, 2);
			float yRemainder = mod(screenFinalPosition.y, 2);
			if(yRemainder == 1.0f)
			{
				if(xRemainder == 1.0f)
				{
					if(i != 0 && i != 4)
					{
						optimizationSkip[i] = true;
					}
					else
					{
						optimizationSkip[i] = false;
					}
				}
				else
				{
					if(i != 1 && i != 5)
					{
						optimizationSkip[i] = true;
					}
					else
					{
						optimizationSkip[i] = false;
					}
				}
			}
			else
			{
				if(xRemainder == 1.0f)
				{
					if(i != 2 && i != 6)
					{
						optimizationSkip[i] = true;
					}
					else
					{
						optimizationSkip[i] = false;
					}
				}
				else
				{
					if(i != 3)
					{
						optimizationSkip[i] = true;
					}
					else
					{
						optimizationSkip[i] = false;
					}
				}
			}	
		}
	}

	vec3 photonViewPos = (view * vec4(texture(purplePhotonPositions, texPos).rgb, 1.0f)).xyz;
	float di = sqrt(dot(photonViewPos, photonViewPos));

	vec3 adjacentPhotonsWorldPos[4] = getRefractedAdjacentPhotonsWorldPos(texPos);
	if(!adjacentPhotonsFound)
	{
		return;
	}

	vec2 adjacentPhotonsWorldPosXY[4];
	for(int photonIndex = 0; photonIndex < 4; photonIndex++)
	{
		adjacentPhotonsWorldPosXY[photonIndex] = adjacentPhotonsWorldPos[photonIndex].xy;
	}

	float Ai = calculateMaxDistanceForAdjacentPhotons(adjacentPhotonsWorldPosXY);

	float ri;
	if(!useCustomSolidAngle)
	{
		ri = sqrt( Ai / (solidAngle * di * di) );
	}
	else
	{
		ri = sqrt( Ai / (customSolidAngle * di * di) );
	}

	float maxDistance = 1.0f;
	if(!blanchetteGapFillingOverride)
	{
		for(int i = 0; i < 6; i++)
		{
			if(ccdOptimizationEnabled && optimizationSkip[i] && photonCCDBasedCalculationEnabled)
			{
				continue;
			}
			for(int j = i + 1; j < 7; j++)
			{
				if(ccdOptimizationEnabled && optimizationSkip[j] && photonCCDBasedCalculationEnabled)
				{
					continue;
				}

				float currentDistance = distance(getScreenPosition(NDCPos[i].xy), 
												 getScreenPosition(NDCPos[j].xy));
				if(currentDistance > maxDistance)
				{
					maxDistance = currentDistance;
				}

				break;
			}
		}
	}

	for(int i = 0; i < 7; i++)
	{
		if(ccdOptimizationEnabled && optimizationSkip[i])
		{
			continue;
		}

		float Di = 2 * ri;
		if(!blanchetteGapFillingOverride)
		{
			if(Di < maxDistance)
			{
				Di = maxDistance;
			}
		}
		int photonMipmapLevel = int( trunc( log2( max(1.0f, Di / 7.0f) ) ) );
		float rawSplatSize = Di / pow(2, photonMipmapLevel);

		int correctSplatSize = getCorrectSplatSize(rawSplatSize);
		if(correctSplatSize > 7)
		{
			correctSplatSize = 7;
		}
		gl_PointSize = correctSplatSize;
		splatSize = correctSplatSize;
		gl_Layer = photonMipmapLevel;

		vec4 finalPosition = NDCPos[i];
		if(photonMipmapLevel > 0)
		{
			finalPosition /= pow(2.0f, photonMipmapLevel);
			finalPosition.w = 1.0f;
		}
		vec4 offsetForMipmapLevel = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		for(int level = 1; level <= photonMipmapLevel; level++)
		{
			offsetForMipmapLevel -= vec4(1.0f / pow(2.0f, level),
									     1.0f / pow(2.0f, level),
										 0.0f,
										 0.0f);
		}
		finalPosition += offsetForMipmapLevel;

		vec2 finalScreenPos = vec2( ((viewportSize.x / 2.0f) * finalPosition.x) + (viewportSize.x / 2.0f),
									((viewportSize.y / 2.0f) * finalPosition.y) + (viewportSize.y / 2.0f));
		finalScreenPos.x = int(finalScreenPos.x);
		finalScreenPos.y = int(finalScreenPos.y);
		centerPosScreen = finalScreenPos;

		gl_Position = finalPosition;
		color = colors[i] / pow(4, photonMipmapLevel);
		EmitVertex();
		EndPrimitive();
	}
	
}