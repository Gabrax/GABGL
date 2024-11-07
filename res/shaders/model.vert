#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT{
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
} vs_out;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;  

    // Tangent space matrix (TBN)
    vec3 T = normalize(mat3(model) * tangent);
    vec3 B = normalize(mat3(model) * bitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    vs_out.TBN = mat3(T, B, N);

    // Tangent-space position for fragment shader
    vs_out.TBN_FragPos = vs_out.TBN * vs_out.FragPos;

    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
    vs_out.TexCoords = aTexCoords;
}
