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
  mat4 ViewProjection;
	mat4 OrtoProjection;
	mat4 NonRotViewProjection;
  vec3 CameraPos;
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

        gl_Position = ViewProjection * modelMat * totalPosition;
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

        gl_Position = ViewProjection * worldPos;
        vs_out.TexCoords = aTexCoords;
    }
}

#type FRAGMENT
#version 450 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
	mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};

struct Material {
    sampler2D diffuse;
    sampler2D normalMap;  // Normal map texture
    vec3 specular;    
    float shininess;
};

struct Light {
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

in VS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
} fs_in;

layout(std430, binding = 0) buffer LightPositions {
    vec4 positions[30];  
};
layout(std430, binding = 1) buffer LightsQuantity {
    int numLights;      
};
layout(std430, binding = 2) buffer LightColors {
    vec4 colors[30];  
};
layout(std430, binding = 3) buffer LightTypes {
    int lightTypes[];
};

uniform Material material;
uniform Light light;

const float gamma = 2.2;

float gammaCorrection(float value) {
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
    return pow(value, vec3(1.0 / gamma));
}

// ACES Tone Mapping
vec3 toneMappingACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * material.specular;

    return ambient + diffuse + specular;
}

vec3 calculatePointLight(Light light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * color * attenuation;
    vec3 diffuse = light.diffuse * diff * color * attenuation;
    vec3 specular = light.specular * spec * material.specular * attenuation;

    return ambient + diffuse + specular;
}

vec3 calculateSpotlight(Light light, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 color) {
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

void main() {
    vec3 color = texture(material.diffuse, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(CameraPos - fs_in.FragPos);

    vec3 lighting = vec3(0.0);

    for (int i = 0; i < numLights; i++) {
        Light currentLight;
        currentLight.position = positions[i].xyz;
        currentLight.diffuse = colors[i].rgb;
        currentLight.constant = 1.0;
        currentLight.linear = 0.09;
        currentLight.quadratic = 0.032;
        currentLight.direction = vec3(0.0f,-1.0f,0.0f);
        currentLight.cutOff = cos(radians(12.5));
        currentLight.outerCutOff = cos(radians(17.5));
        
        if (lightTypes[i] == 0) { 
            lighting += calculatePointLight(currentLight, fs_in.FragPos, normal, viewDir, color);
        } else if (lightTypes[i] == 1) { 
            lighting += calculateDirectionalLight(currentLight, normal, viewDir, color);
        } else if (lightTypes[i] == 2) { 
            lighting += calculateSpotlight(currentLight, fs_in.FragPos, normal, viewDir, color);
        }
    }

    vec3 result = lighting;

    result = toneMappingACES(result);
    // result = gammaCorrection(result);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor = vec4(result, 1.0);
}


