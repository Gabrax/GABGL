#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform float renderWidth;
uniform float renderHeight;

uniform float pixelWidth = 5.0;
uniform float pixelHeight = 5.0;

void main()
{
    float dx = pixelWidth*(1.0/renderWidth);
    float dy = pixelHeight*(1.0/renderHeight);

    vec2 coord = vec2(dx*floor(TexCoords.x/dx), dy*floor(TexCoords.y/dy));

    vec3 tc = texture(screenTexture, coord).rgb;

    FragColor = vec4(tc, 1.0);
}
