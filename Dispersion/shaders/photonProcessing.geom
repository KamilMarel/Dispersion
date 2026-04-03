#version 330 core
layout (points) in;
layout (points, max_vertices = 4) out;

uniform float mipmapLevel;
uniform vec2 photonChildOffset;

uniform sampler2D photonPositions;

out vec4 outValue;

void addPhotonForNextMipmapLevel(float offsetX, float offsetY)
{
    vec4 newPosition = gl_in[0].gl_Position + vec4(offsetX, 
                                              offsetY, 
                                              0.0, 
                                              0.0); 
    outValue = newPosition;
    EmitVertex();
    EndPrimitive();
}

bool checkIfPhotonHitsTheRefractor()
{
    vec2 photonPositionInTextureSpace = vec2((gl_in[0].gl_Position.x + 1) / 2.0f, 
                                             (gl_in[0].gl_Position.y + 1) / 2.0f);
    
    if(textureLod(photonPositions, photonPositionInTextureSpace, mipmapLevel) == vec4(1.0f, 1.0f, 1.0f, 1.0f))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void main() 
{
    if(gl_in[0].gl_Position != vec4(0.0f, 0.0f, 0.0f, 0.0f))
    {
        if(checkIfPhotonHitsTheRefractor())
        {
            addPhotonForNextMipmapLevel(photonChildOffset.x, photonChildOffset.y);
            addPhotonForNextMipmapLevel(photonChildOffset.x, -photonChildOffset.y);
            addPhotonForNextMipmapLevel(-photonChildOffset.x, -photonChildOffset.y);
            addPhotonForNextMipmapLevel(-photonChildOffset.x, photonChildOffset.y);
        }
    }
}