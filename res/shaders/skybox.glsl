#type VERTEX
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 u_ViewProjection;

void main()
{
    TexCoords = aPos;
    vec4 pos = u_ViewProjection * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  

#type FRAGMENT
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

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

void main()
{
    vec3 skyTexture = texture(skybox, TexCoords).rgb;

    FragColor = vec4(skyTexture, 1.0);
}

