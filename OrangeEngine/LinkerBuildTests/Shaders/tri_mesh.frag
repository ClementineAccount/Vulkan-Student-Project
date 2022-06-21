#version 450

layout(binding = 1) uniform sampler2D texSampler[5];

//[0] = albedo
//[1] = normalmap texture
//[2] = roughnessMap texture
//[3] = aoMap Texture
//[4] = metallic texture


const float PI = 3.14159265359;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 cameraPos;
layout(location = 5) in vec3 lightColor;
layout(location = 6) in vec3 localPos;
layout(location = 7) in vec3 tangentViewPos;
layout(location = 8) in vec3 tangentLightPos;
layout(location = 9) in vec3 tangentFragPos;
layout (location = 10) in vec3 normal;

layout(location = 0) out vec4 outColor;



void main() {

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * texture(texSampler[0], fragTexCoord).rgb;  

    
    
    // specular
    float specularStrength = 1.0;


    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular);
    outColor = vec4(result, 1.0);
}