#version 330 core

out vec4 FragColor;

in vec3 color;
flat in int splatSize;
in vec2 centerPosScreen;

uniform float coefs3x3[9];
uniform float coefs7x7[49];
uniform float coefs11x11[121];
uniform float coefs15x15[225];

void main()
{   
    int xPosPoint = int(gl_FragCoord.x - centerPosScreen.x);
    int yPosPoint = int(gl_FragCoord.y - centerPosScreen.y);

    float colorMod = 1.0f;

    switch(splatSize)
    {
        case 3:
        {
            int xIndex = xPosPoint + 1;
            int yIndex = abs(yPosPoint - 1);
            colorMod = coefs3x3[(3 * yIndex) + xIndex];
            break;
        }
        case 7:
        {
            int xIndex = xPosPoint + 3;
            int yIndex = abs(yPosPoint - 3);
            colorMod = coefs7x7[(7 * yIndex) + xIndex];
            break;
        }
        case 11:
        {
            int xIndex = xPosPoint + 5;
            int yIndex = abs(yPosPoint - 5);
            colorMod = coefs11x11[(11 * yIndex) + xIndex];
            break;
        }
        case 15:
        {
            int xIndex = xPosPoint + 7;
            int yIndex = abs(yPosPoint - 7);
            colorMod = coefs15x15[(15 * yIndex) + xIndex];
            break;
        }
        default:
            break;
    }
    FragColor = vec4(color * colorMod, 1.0f);
}