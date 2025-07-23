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
#version 460 core
#extension GL_ARB_bindless_texture : require

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gAlbedoSpec;
layout(binding = 4) uniform sampler2D u_DirectShadow;
layout(binding = 5) uniform sampler3D u_OffsetTexture;
layout(binding = 6) uniform samplerCubeArray u_OmniShadow;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
  mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(std140, binding = 2) uniform DirectShadowData
{
  vec2 windowSize;
  vec2 offsetSize_filterSize;
  vec2 randomRadius;
};

layout(std430, binding = 0) buffer LightPositions    { vec4 positions[]; };
layout(std430, binding = 1) buffer LightRotations    { vec4 rotations[]; };
layout(std430, binding = 2) buffer LightsQuantity    { int numLights;    };
layout(std430, binding = 3) buffer LightColors       { vec4 colors[];    };
layout(std430, binding = 4) buffer LightTypes        { int lightTypes[]; };

uniform mat4 u_DirectShadowViewProj;

const float gamma = 1.2;

float gammaCorrection(float value) {
  return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
  return pow(value, vec3(1.0 / gamma));
}

// ACES tone mapping
vec3 toneMappingACES(vec3 color) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

float calculateDirectShadow(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal) {
  ivec3 OffsetCoord;
  vec2 f = mod(gl_FragCoord.xy, offsetSize_filterSize.xx);
  OffsetCoord.yz = ivec2(f);

  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  vec3 shadowCoords = projCoords * 0.5 + 0.5;

  float texelSizeX = 1.0 / windowSize.x;
  float texelSizeY = 1.0 / windowSize.y;
  vec2 texelSize = vec2(texelSizeX, texelSizeY);

  float diffuseFactor = dot(normal, -lightDir);
  float bias = mix(0.001, 0.0, diffuseFactor);

  float sum = 0.0;
  int sampleCount = int(offsetSize_filterSize.y * offsetSize_filterSize.y);
  int samplesDiv2 = sampleCount / 2;

  for (int i = 0; i < samplesDiv2; ++i) {
    OffsetCoord.x = i;
    vec4 offsets = texelFetch(u_OffsetTexture, OffsetCoord, 0) * randomRadius.x;

    vec2 offsets1 = shadowCoords.xy + offsets.rg * texelSize;
    float depth1 = texture(u_DirectShadow, offsets1).r;
    sum += (depth1 + bias < shadowCoords.z) ? 0.15 : 1.0;

    vec2 offsets2 = shadowCoords.xy + offsets.ba * texelSize;
    float depth2 = texture(u_DirectShadow, offsets2).r;
    sum += (depth2 + bias < shadowCoords.z) ? 0.15 : 1.0;
  }

  return sum / float(sampleCount);
}

float calculatePointShadow(vec3 fragPos, vec3 lightPos, vec3 normal, int lightIndex) {
  vec3 fragToLight = fragPos - lightPos;
  float currentDepth = length(fragToLight);
  vec3 direction = normalize(fragToLight);

  float closestDepth = texture(u_OmniShadow, vec4(direction, float(lightIndex))).r;
  float bias = max(0.05 * (1.0 - dot(normal, direction)), 0.005);

  return (currentDepth - bias > closestDepth) ? 0.15 : 1.0;
}

vec3 calculateDirectionalLight(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 surfaceColor, vec4 fragPosLightSpace, vec3 ambientCol, vec3 specularCol, vec3 lightColor, float shininess) {
  lightDir = normalize(-lightDir);
  float shadow = calculateDirectShadow(fragPosLightSpace, lightDir, normal);
  float diff = max(dot(normal, lightDir), 0.0);

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

  vec3 ambient = ambientCol * surfaceColor;
  vec3 diffuse = lightColor * diff * surfaceColor * shadow;
  vec3 specular = specularCol * spec * shadow;

  return ambient + diffuse + specular;
}

vec3 calculatePointLight(vec3 lightPos, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 surfaceColor, vec3 ambientCol, vec3 specularCol, int lightIndex, vec3 lightColor, float shininess) {
  vec3 lightDir = normalize(lightPos - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);

  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

  float distance = length(lightPos - fragPos);
  float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
  float shadow = calculatePointShadow(fragPos, lightPos, normal, lightIndex);

  vec3 ambient = ambientCol * surfaceColor * attenuation;
  vec3 diffuse = lightColor * diff * surfaceColor * shadow * attenuation;
  vec3 specular = specularCol * spec * shadow * attenuation;

  return ambient + diffuse + specular;
}

vec3 calculateSpotlight(vec3 lightPos, vec3 lightDir, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 surfaceColor, vec3 ambientCol, vec3 specularCol, vec3 lightColor, float shininess) {
  vec3 lightToFrag = normalize(fragPos - lightPos);
  vec3 normLightDir = normalize(lightDir); // Light direction points "outward"

  float theta = dot(lightToFrag, normLightDir);
  float cutOff = cos(radians(12.5));
  float outerCutOff = cos(radians(17.5));
  float epsilon = cutOff - outerCutOff;
  float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

  float diff = max(dot(normal, -lightToFrag), 0.0);

  vec3 reflectDir = reflect(lightToFrag, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

  float distance = length(lightPos - fragPos);
  float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

  vec3 ambient  = ambientCol  * surfaceColor;
  vec3 diffuse  = lightColor  * diff * surfaceColor;
  vec3 specular = specularCol * spec;

  vec3 result = (ambient + diffuse + specular) * attenuation * intensity;
  return result;
}

void main()
{
  vec3 FragPos = texture(gPosition, TexCoords).rgb;
  vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
  vec4 AlbedoSpec = texture(gAlbedoSpec, TexCoords);
  vec3 Color = AlbedoSpec.rgb;
  vec3 viewDir = normalize(CameraPos - FragPos);
  vec4 fragPosLightSpace = u_DirectShadowViewProj * vec4(FragPos, 1.0);
  vec3 specularCol = vec3(AlbedoSpec.a);

  float shininess = mix(8.0, 128.0, AlbedoSpec.a);

  vec3 result = vec3(0.0);

  for (int i = 0; i < numLights; ++i)
  {
    vec3 position = positions[i].xyz;
    vec3 rotation = rotations[i].xyz;
    vec3 lightColor = colors[i].rgb;
    vec3 ambient = lightColor * 0.1;

    int type = lightTypes[i];

    if (type == 0) {
      result += calculateDirectionalLight(rotation, Normal, viewDir, Color, fragPosLightSpace, ambient, specularCol, lightColor, shininess);
    } else if (type == 1) {
      result += calculatePointLight(position, FragPos, Normal, viewDir, Color, ambient, specularCol, i, lightColor, shininess);
    } else if (type == 2) {
      result += calculateSpotlight(position, rotation, FragPos, Normal, viewDir, Color, ambient, specularCol, lightColor, shininess);
    }
  }

  result = toneMappingACES(result);
  result = gammaCorrection(result);

  float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
  BrightColor = brightness > 0.7 ? vec4(result, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
  FragColor = vec4(result, 1.0);
}


