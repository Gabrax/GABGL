#type VERTEX
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

#type FRAGMENT
#version 430 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct Material {
    sampler2D diffuse;
    sampler2D normalMap;  // Normal map texture
    vec3 specular;    
    float shininess;
};

// Light structure to hold properties like ambient, diffuse, etc.
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

layout(std430, binding = 3) buffer LightsQuantity {
    int numLights;      
};
layout(std430, binding = 4) buffer LightPositions {
    vec4 positions[10];  
};
layout(std430, binding = 5) buffer LightColors {
    vec4 colors[10];  
};
layout(std430, binding = 6) buffer LightTypes {
    int lightTypes[];
};

uniform vec3 viewPos;  // Camera position
uniform Material material;
uniform Light light;    // Shared light properties for all lights

const float gamma = 2.2;

// Gamma correction function
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

// ACES tone mapping curve
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

bool isInsideAABB(vec3 fragPos, vec3 minAABB, vec3 maxAABB) {
    return all(greaterThanEqual(fragPos, minAABB)) && all(lessThanEqual(fragPos, maxAABB));
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
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

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

    // Apply tone mapping and gamma correction
    result = toneMappingACES(result);
    // result = gammaCorrection(result);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor = vec4(result, 1.0);
}

// void main() {
//     vec3 totalAmbient = vec3(0.0);
//     vec3 totalDiffuse = vec3(0.0);
//     vec3 totalSpecular = vec3(0.0);
//
//     // Define ratios for ambient, diffuse, and specular
//     const float ambientRatio = 0.3;
//     const float diffuseRatio = 0.6;
//     const float specularRatio = 0.1;
//
//     for (int i = 0; i < numLights; i++) {
//         vec3 lightPos = positions[i].xyz;  // Access the light position from the SSBO
//
//         // Split color into ambient, diffuse, and specular components
//         vec3 lightColor = colors[i].rgb;
//         vec3 ambientColor = lightColor * ambientRatio;
//         vec3 diffuseColor = lightColor * diffuseRatio;
//         vec3 specularColor = lightColor * specularRatio;
//
//         // Calculate ambient lighting
//         vec3 ambient = ambientColor * texture(material.diffuse, fs_in.TexCoords).rgb;
//
//         // Normal mapping: get perturbed normal
//         vec3 normal = texture(material.normalMap, fs_in.TexCoords).rgb;
//         normal = normalize(normal * 2.0 - 1.0);  // Transform to range [-1, 1]
//         vec3 perturbedNormal = normalize(fs_in.TBN * normal);  // Convert to world space
//
//         // Calculate light direction for the current light position
//         vec3 lightDir = normalize(lightPos - fs_in.FragPos);
//         float diff = max(dot(perturbedNormal, lightDir), 0.0);
//         vec3 diffuse = diffuseColor * diff * texture(material.diffuse, fs_in.TexCoords).rgb;
//
//         // Specular lighting (Blinn-Phong)
//         vec3 viewDir = normalize(viewPos - fs_in.FragPos);
//         vec3 halfwayDir = normalize(lightDir + viewDir);  // Blinn-Phong halfway vector
//         float spec = pow(max(dot(perturbedNormal, halfwayDir), 0.0), material.shininess);
//         vec3 specular = specularColor * (spec * material.specular);
//
//         // Calculate attenuation based on distance to light source
//         float distance = length(lightPos - fs_in.FragPos);
//         float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
//
//         // Apply attenuation to each light's contribution
//         ambient  *= attenuation;
//         diffuse  *= attenuation;
//         specular *= attenuation;
//
//         // Accumulate each light’s isolated effect to the totals
//         totalAmbient  += ambient;
//         totalDiffuse  += diffuse;
//         totalSpecular += specular;
//     }
//
//     // Combine all lighting components
//     vec3 result = totalAmbient + totalDiffuse + totalSpecular;
//
//     // Apply tone mapping
//     result = toneMappingACES(result);
//
//     // Apply gamma correction
//     result = gammaCorrection(result);
//
//     // Output the final color
//     FragColor = vec4(result, 1.0);
// }

// void main()
// {           
//     vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
//     vec3 normal = normalize(fs_in.Normal);
//     // ambient
//     vec3 ambient = 0.0 * color;
//     // lighting
//     vec3 lighting = vec3(0.0);
//     vec3 viewDir = normalize(viewPos - fs_in.FragPos);
//     for(int i = 0; i < numLights; i++)
//     {
//         // diffuse
//         vec3 lightDir = normalize(positions[i].xyz - fs_in.FragPos);
//         float diff = max(dot(lightDir, normal), 0.0);
//         vec3 result = lights[i].Color * diff * color;      
//         // attenuation (use quadratic as we have gamma correction)
//         float distance = length(fs_in.FragPos - positions[i].xyz);
//         result *= 3.0 / (distance * distance);
//         lighting += result;
//
//     }
//     vec3 result = ambient + lighting;
//     // check whether result is higher than some threshold, if so, output as bloom threshold color
//     float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
//     if(brightness > 1.0)
//         BrightColor = vec4(result, 1.0);
//     else
//         BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
//     FragColor = vec4(result, 1.0);
// }
