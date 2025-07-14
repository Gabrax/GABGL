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

out VS_OUT{
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
    vec4 FragPosLightSpace;
    flat uint DrawID;
} vs_out;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 9) buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) buffer ModelIsAnimated  { int modelIsAnimated[]; };

uniform bool isInstanced;

uniform mat4 u_DirectShadowViewProj;

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
    vs_out.FragPos = vec3(worldPos);
    vs_out.Normal = mat3(transpose(inverse(modelMat))) * aNormal;

    vec3 T = normalize(mat3(modelMat) * tangent);
    vec3 B = normalize(mat3(modelMat) * bitangent);
    vec3 N = normalize(mat3(modelMat) * aNormal);
    vs_out.TBN = mat3(T, B, N);
    vs_out.TBN_FragPos = vs_out.TBN * vs_out.FragPos;

    gl_Position = ViewProjection * worldPos;
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = u_DirectShadowViewProj * worldPos;
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

    gl_Position = ViewProjection * worldPos;
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = u_DirectShadowViewProj * vec4(worldPos.xyz, 1.0);
  }
}

#type FRAGMENT
#version 460 core
#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

layout(binding = 1) uniform sampler2D u_DirectShadow;
layout(binding = 2) uniform sampler3D u_OffsetTexture;
layout(binding = 3) uniform samplerCubeArray u_OmniShadow;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
	mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(std140, binding = 1) uniform DirectShadowData
{
  vec2 windowSize;
  vec2 offsetSize_filterSize; // x = offsetSize, y = filterSize 
  vec2 randomRadius; // x = randomRadius
};

layout(std430, binding = 0) buffer LightPositions    { vec4 positions[30]; };
layout(std430, binding = 1) buffer LightRotations    { vec4 rotations[30]; };
layout(std430, binding = 2) buffer LightsQuantity    { int numLights; };
layout(std430, binding = 3) buffer LightColors       { vec4 colors[30]; };
layout(std430, binding = 4) buffer LightTypes        { int lightTypes[]; };
layout(std430, binding = 7) buffer MeshTextures      { sampler2D meshTextures[]; };
layout(std430, binding = 8) buffer MeshTextureRanges { uvec2 meshTextureRanges[]; }; // .x = start, .y = count

struct Material
{
  sampler2D diffuse;
  sampler2D normalMap;  
  vec3 specular;    
  float shininess;
};

Material getMaterial(uint drawID)
{
  Material mat;
  uvec2 texRange = meshTextureRanges[drawID];
  
  mat.diffuse = meshTextures[texRange.x];
  mat.normalMap = meshTextures[texRange.x + 1];

  // mat.specular = vec3(0.5); 
  // mat.shininess = 32.0;     

  return mat;
}

struct Light
{
  vec3 position;    // For point light and spotlight
  vec3 direction;   // For directional and spotlight
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;   // Point and spotlight attenuation
  float linear;     
  float quadratic;

  float cutOff;       // Spotlight inner cutoff (cosine)
  float outerCutOff;  // Spotlight outer cutoff (cosine)

  int type;  // 0 = Directional, 1 = Point, 2 = Spotlight
};

uniform Material material;
uniform Light light;

in VS_OUT
{
  vec2 TexCoords;
  vec3 Normal;
  vec3 FragPos;
  vec3 TBN_FragPos;
  mat3 TBN;
  vec4 FragPosLightSpace;
  flat uint DrawID;
} fs_in;

const float gamma = 2.2;

float gammaCorrection(float value) {
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
    return pow(value, vec3(1.0 / gamma));
}

// ACES Tone Mapping
vec3 toneMappingACES(vec3 color)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;

  return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

float CalcShadowFactorWithRandomSampling(vec4 fragPosLightSpace, vec3 LightDirection, vec3 Normal)
{
  ivec3 OffsetCoord;
  vec2 f = mod(gl_FragCoord.xy, vec2(offsetSize_filterSize.x));
  OffsetCoord.yz = ivec2(f);

  float Sum = 0.0;

  int SamplesDiv2 = int(offsetSize_filterSize.y * offsetSize_filterSize.y / 2.0);

  vec3 ProjCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  vec3 ShadowCoords = ProjCoords * 0.5 + vec3(0.5);

  vec4 sc = vec4(ShadowCoords, 1.0);

  float TexelWidth = 1.0 / windowSize.x;
  float TexelHeight = 1.0 / windowSize.y;

  vec2 TexelSize = vec2(TexelWidth, TexelHeight);

  float DiffuseFactor = dot(Normal, -LightDirection);
  float bias = mix(0.001, 0.0, DiffuseFactor);
  float Depth = 0.0;

  for (int i = 0 ; i < 4 ; i++)
  {
    OffsetCoord.x = i;
    vec4 Offsets = texelFetch(u_OffsetTexture, OffsetCoord, 0) * randomRadius.x;
    sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
    Depth = texture(u_DirectShadow, sc.xy).x;

    Sum += (Depth + bias < ShadowCoords.z) ? 0.15 : 1.0;

    sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
    Depth = texture(u_DirectShadow, sc.xy).x;

    Sum += (Depth + bias < ShadowCoords.z) ? 0.15 : 1.0;
  }

  float Shadow = Sum / 8.0;

  if (Shadow != 0.0 && Shadow != 1.0)
  {
    for (int i = 4 ; i < SamplesDiv2 ; i++)
    {
      OffsetCoord.x = i;
      vec4 Offsets = texelFetch(u_OffsetTexture, OffsetCoord, 0) * randomRadius.x;
      sc.xy = ShadowCoords.xy + Offsets.rg * TexelSize;
      Depth = texture(u_DirectShadow, sc.xy).x;

      Sum += (Depth + bias < ShadowCoords.z) ? 0.15 : 1.0;

      sc.xy = ShadowCoords.xy + Offsets.ba * TexelSize;
      Depth = texture(u_DirectShadow, sc.xy).x;

      Sum += (Depth + bias < ShadowCoords.z) ? 0.15 : 1.0;
    }

    Shadow = Sum / float(SamplesDiv2 * 2.0);
  }

  return Shadow;
}

vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 viewDir, vec3 color, vec4 fragPosLightSpace)
{
  vec3 lightDir = normalize(-light.direction);

  float shadow = CalcShadowFactorWithRandomSampling(fragPosLightSpace, lightDir, normal);

  float diff = max(dot(normal, lightDir), 0.0);

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

  vec3 ambient = light.ambient * color;
  vec3 diffuse = light.diffuse * diff * color * shadow;     
  vec3 specular = light.specular * spec * material.specular * shadow; 

  return ambient + diffuse + specular;
}

float calculatePointShadow(vec3 fragPos, vec3 lightPos, int lightIndex)
{
  vec3 fragToLight = fragPos - lightPos;
  float currentDepth = length(fragToLight);

  vec3 direction = normalize(fragToLight);

  float closestDepth = texture(u_OmniShadow, vec4(direction, float(lightIndex))).r;

  float bias = max(0.05 * (1.0 - dot(normalize(fs_in.Normal), direction)), 0.005);

  float shadow = currentDepth - bias > closestDepth ? 0.15 : 1.0;

  return shadow;
}

vec3 calculatePointLight(Light light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 color, int lightIndex)
{
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  // Shadow calculation with cubemap array
  float shadow = calculatePointShadow(fragPos, light.position, lightIndex);

  vec3 ambient = light.ambient * color * attenuation;
  vec3 diffuse = light.diffuse * diff * color * attenuation * shadow;
  vec3 specular = light.specular * spec * material.specular * attenuation * shadow;

  return ambient + diffuse + specular;
}

vec3 calculateSpotlight(Light light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 color)
{
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  float theta = dot(lightDir, normalize(-light.direction));
  float epsilon = light.cutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

  vec3 ambient = light.ambient * color * attenuation * intensity;
  vec3 diffuse = light.diffuse * diff * color * attenuation * intensity;
  vec3 specular = light.specular * spec * material.specular * attenuation * intensity;

  return ambient + diffuse + specular;
}

void main()
{
  Material material = getMaterial(fs_in.DrawID);
  vec3 color = texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 normal = normalize(fs_in.Normal);
  vec3 viewDir = normalize(CameraPos - fs_in.FragPos);

  vec3 lighting = vec3(0.0);

  for (int i = 0; i < numLights; i++)
  {
    Light currentLight;
    currentLight.position = positions[i].xyz;
    currentLight.diffuse = colors[i].rgb;
    currentLight.ambient = currentLight.diffuse * 0.1f;
    currentLight.constant = 1.0;
    currentLight.linear = 0.09;
    currentLight.quadratic = 0.032;
    currentLight.direction = rotations[i].xyz;
    currentLight.cutOff = cos(radians(12.5));
    currentLight.outerCutOff = cos(radians(17.5));
    
    if (lightTypes[i] == 0)
    { 
      lighting += calculatePointLight(currentLight, fs_in.FragPos, normal, viewDir, color,i);
    } 
    else if (lightTypes[i] == 1)
    { 
      lighting += calculateDirectionalLight(currentLight, normal, viewDir, color, fs_in.FragPosLightSpace);
    } 
    else if (lightTypes[i] == 2)
    { 
      lighting += calculateSpotlight(currentLight, fs_in.FragPos, normal, viewDir, color);
    }
  }

  vec3 result = lighting;

  result = toneMappingACES(result);
  // result = gammaCorrection(result);

  float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
  if (brightness > 1.0) BrightColor = vec4(result, 1.0);
  else BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

  FragColor = vec4(result, 1.0);
}


