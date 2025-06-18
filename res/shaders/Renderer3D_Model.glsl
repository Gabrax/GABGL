#type VERTEX
#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;
layout (location = 7) in mat4 instanceMatrix;

uniform mat4 model;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

out VS_OUT{
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
} vs_out;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform bool isAnimated;
uniform bool isInstanced;

void main()
{
    mat4 modelMat = isInstanced ? instanceMatrix : model;

    if (isAnimated)
    {
        vec4 totalPosition = vec4(0.0f);
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (boneIds[i] == -1) continue;
            if (boneIds[i] >= MAX_BONES)
            {
                totalPosition = vec4(aPos, 1.0f);
                break;
            }
            vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos, 1.0f);
            totalPosition += localPosition * weights[i];
        }

        vs_out.FragPos = vec3(modelMat * totalPosition);
        vs_out.Normal = mat3(transpose(inverse(modelMat))) * aNormal;

        vec3 T = normalize(mat3(modelMat) * tangent);
        vec3 B = normalize(mat3(modelMat) * bitangent);
        vec3 N = normalize(mat3(modelMat) * aNormal);
        vs_out.TBN = mat3(T, B, N);
        vs_out.TBN_FragPos = vs_out.TBN * vs_out.FragPos;

        gl_Position = u_ViewProjection * modelMat * totalPosition;
        vs_out.TexCoords = aTexCoords;
    }
    else
    {
        vec4 worldPos = modelMat * vec4(aPos, 1.0);
        vs_out.FragPos = vec3(worldPos);
        vs_out.Normal = mat3(transpose(inverse(modelMat))) * aNormal;

        vec3 T = normalize(mat3(modelMat) * tangent);
        vec3 B = normalize(mat3(modelMat) * bitangent);
        vec3 N = normalize(mat3(modelMat) * aNormal);
        vs_out.TBN = mat3(T, B, N);
        vs_out.TBN_FragPos = vs_out.TBN * vs_out.FragPos;

        gl_Position = u_ViewProjection * worldPos;
        vs_out.TexCoords = aTexCoords;
    }
}

#type FRAGMENT
#version 450 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct Material {
    sampler2D diffuse;
    sampler2D normalMap;  // Normal map texture
    vec3 specular;    
    float shininess;
};

in VS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
} fs_in;

uniform Material material;

void main() {
    vec3 color = texture(material.diffuse, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);

    FragColor = vec4(color, 1.0);
}

