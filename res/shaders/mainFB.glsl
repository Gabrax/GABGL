#type VERTEX
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  

#type FRAGMENT
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;  // Main scene texture
uniform sampler2D screenTexture2; // Bloom texture

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

