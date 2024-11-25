#type VERTEX
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}

#type FRAGMENT
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform sampler2D skyboxTexture;
uniform float exposure = 5.0f;
uniform float bloomStrength = 0.54f;

uniform float renderWidth;
uniform float renderHeight;

uniform float pixelWidth = 5.0;
uniform float pixelHeight = 5.0;

// ACES Tone Mapping
vec3 toneMappingACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

// ACES tone mapping curve
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

const float gamma = 2.2;

// Gamma correction function
float gammaCorrection(float value)
{
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value)
{
    return pow(value, vec3(1.0 / gamma));
}

vec3 ApplyBloom()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    return mix(hdrColor, bloomColor, bloomStrength); // linear interpolation
}

void main()
{
    // float dx = pixelWidth*(1.0/renderWidth);
    // float dy = pixelHeight*(1.0/renderHeight);
    //
    // vec2 coord = vec2(dx*floor(TexCoords.x/dx), dy*floor(TexCoords.y/dy));
    //
    // vec4 t1 = texture(scene, coord);
    // vec4 t2 = texture(bloomBlur, coord);

    vec3 result = vec3(0.0);
    result = ApplyBloom();
    result = toneMappingACES(result * exposure);
    result = gammaCorrection(result);
    FragColor = vec4(result, 1.0);
}
