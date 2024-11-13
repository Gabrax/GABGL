#version 430 core

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D normalMap;  // Normal map texture
    vec3 specular;    
    float shininess;
};

struct Light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in VS_OUT{
    vec2 TexCoords;
    vec3 FragPos;
    vec3 TBN_FragPos;
    mat3 TBN;
} fs_in;

// SSBO for storing light positions, bound to binding point 0
layout(std430, binding = 3) buffer LightPositions {
    int numLights;      // Number of lights
    vec3 positions[100];  // Array of light positions
};


uniform vec3 viewPos;  // Camera position
uniform Material material;
uniform Light light;

// Gamma correction parameters
const float gamma = 2.2;

float gammaCorrection(float value) {
    return pow(value, 1.0 / gamma);
}

vec3 gammaCorrection(vec3 value) {
    return pow(value, vec3(1.0 / gamma));
}

void main() {
    vec3 result = vec3(0.0);

    // Loop over all lights
    for (int i = 0; i < numLights; i++) {
        vec3 lightPos = positions[i];  // Access the light position from the SSBO

        // Ambient lighting
        vec3 ambient = light.ambient * texture(material.diffuse, fs_in.TexCoords).rgb;

        // Normal mapping: get perturbed normal
        vec3 normal = texture(material.normalMap, fs_in.TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);  // Transform to range [-1, 1]
        vec3 perturbedNormal = normalize(fs_in.TBN * (normal * 2.0 - 1.0));  // Transform to world space

        // Diffuse lighting with normal mapping
        vec3 lightDir = normalize(fs_in.TBN_FragPos * (lightPos - fs_in.FragPos));
        float diff = max(dot(perturbedNormal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texture(material.diffuse, fs_in.TexCoords).rgb;

        // Specular lighting (Blinn-Phong) with normal mapping
        vec3 viewDir = normalize(viewPos - fs_in.FragPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);  // Blinn-Phong halfway vector
        float spec = pow(max(dot(perturbedNormal, halfwayDir), 0.0), material.shininess);
        vec3 specular = light.specular * (spec * material.specular);

        // Point light attenuation
        float distance = length(lightPos - fs_in.FragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

        ambient  *= attenuation;
        diffuse  *= attenuation;
        specular *= attenuation;

        // Add light contribution to the result
        result += ambient + diffuse + specular;
    }

    // Gamma correction: apply to the final color before outputting
    result = gammaCorrection(result);  // Apply gamma correction to the final color

    // Output the final color
    FragColor = vec4(result, 1.0);
}
