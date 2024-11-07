#version 330 core

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D normalMap;  // Normal map texture
    vec3 specular;    
    float shininess;
};

struct Light {
    vec3 position;
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

uniform vec3 viewPos;  // Camera position
uniform Material material;
uniform Light light;

void main()
{
    // 1. Ambient lighting
    vec3 ambient = light.ambient * texture(material.diffuse, fs_in.TexCoords).rgb;
    
    // 2. Normal mapping: get perturbed normal
    vec3 normal = texture(material.normalMap, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);  // Transform to range [-1, 1]
    vec3 perturbedNormal = normalize(fs_in.TBN * (normal * 2.0 - 1.0));  // Transform to world space

    // 3. Diffuse lighting with normal mapping
    vec3 lightDir = normalize(fs_in.TBN_FragPos * (light.position - fs_in.FragPos));
    float diff = max(dot(perturbedNormal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, fs_in.TexCoords).rgb;
    
    // 4. Specular lighting (Blinn-Phong) with normal mapping
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  // Blinn-Phong halfway vector
    float spec = pow(max(dot(perturbedNormal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // 5. Point light attenuation
    float distance = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    ambient  *= attenuation;  
    diffuse  *= attenuation;
    specular *= attenuation; 
    
    // Final color computation
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
