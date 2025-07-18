#type VERTEX
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;
layout (location = 7) in mat4 instanceMatrix;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
	mat4 OrtoProjection;
	mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(std430, binding = 5) buffer ModelTransforms    { mat4 transforms[]; };
layout(std430, binding = 6) buffer MeshToTransformMap { int meshToTransform[]; };

out VS_OUT
{
  vec3 FragPos;
  vec3 Normal;
  vec2 TexCoords;
  flat uint DrawID;
} vs_out;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 9) buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) buffer ModelIsAnimated  { int modelIsAnimated[]; };

uniform bool isInstanced;

void main()
{
  vs_out.DrawID = gl_DrawID;
  int transformIndex = meshToTransform[vs_out.DrawID];
  bool isAnimated = (modelIsAnimated[transformIndex] == 1);
  mat4 modelMat = isInstanced ? instanceMatrix : transforms[transformIndex];

  if (isAnimated)
  {
    int boneBaseIndex = transformIndex * MAX_BONES;
    vec4 totalPosition = vec4(0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
      if (boneIds[i] == -1) continue;
      if (boneIds[i] >= MAX_BONES)
      {
          totalPosition = vec4(aPos, 1.0f);
          break;
      }
      vec4 localPosition = boneMatrices[boneBaseIndex + boneIds[i]] * vec4(aPos, 1.0f);
      totalPosition += localPosition * weights[i];
    }

    vec4 worldPos = modelMat * totalPosition;  
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = mat3(transpose(inverse(modelMat))) * aNormal;

    gl_Position = ViewProjection * worldPos;
    vs_out.TexCoords = aTexCoords;
  }
  else
  {
    vec4 worldPos = modelMat * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = mat3(transpose(inverse(modelMat))) * aNormal;

    gl_Position = ViewProjection * worldPos;
    vs_out.TexCoords = aTexCoords;
  }
}

#type FRAGMENT
#version 460 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;

layout(std430, binding = 7) buffer MeshTextures      { sampler2D meshTextures[]; };
layout(std430, binding = 8) buffer MeshTextureRanges { uvec2 meshTextureRanges[]; };

struct Material
{
  sampler2D diffuse;
  sampler2D normalMap;  
  sampler2D specular;    
  float shininess;
};

Material getMaterial(uint drawID)
{
  Material mat;
  uvec2 texRange = meshTextureRanges[drawID];
  mat.diffuse = meshTextures[texRange.x];
  mat.normalMap = meshTextures[texRange.x + 1];
  mat.specular = meshTextures[texRange.x + 2];
  mat.shininess = 32.0;
  return mat;
}

in VS_OUT
{
  vec3 FragPos;
  vec3 Normal;
  vec2 TexCoords;
  flat uint DrawID;
} fs_in;

void main()
{
  Material material = getMaterial(fs_in.DrawID);
  
  gPosition = fs_in.FragPos;
  gNormal = normalize(fs_in.Normal);

  vec3 albedo = texture(material.diffuse, fs_in.TexCoords).rgb;
  gAlbedoSpec.rgb = albedo;
  gAlbedoSpec.a = texture(material.specular, fs_in.TexCoords).r;
}

