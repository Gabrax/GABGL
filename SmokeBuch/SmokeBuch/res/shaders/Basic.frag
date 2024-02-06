#version 330 core


struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
}; 

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out vec4 FragColor;  
in vec3 ourColor;

in vec2 TexCoord;
in vec3 Normal;  
in vec3 FragPos; 

uniform sampler2D Texture1;
uniform sampler2D Texture2;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform Light light;
uniform Material material;
uniform float time;

  
void main()
{
   // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoord).rgb;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoord).rgb;  
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoord).rgb;  
        
    /*Emission */
    vec3 emission = vec3(0.0);
    if (texture(material.specular, TexCoord).r == 0.0)   /*rough check for blackbox inside spec texture */
    {
        /*apply emission texture */
        emission = texture(material.emission, TexCoord).rgb;
        
        /*some extra fun stuff with "time uniform" */
        emission = texture(material.emission, TexCoord + vec2(0.0,time)).rgb;   /*moving */
        emission = emission * (sin(time) * 0.5 + 0.5) * 2.0;                     /*fading */
    }
        
    vec3 result = ambient + diffuse + specular + emission;
    FragColor = vec4(result, 1.0);

}