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
layout (location = 11) in vec3 ambientColor;
layout (location = 12) in mat3 BTN;

layout(location = 0) out vec4 outColor;



void main() {

    vec3 Normal;
    Normal.xy	= (texture(texSampler[1], fragTexCoord).gr * 2.0) - 1.0;
    Normal.z =  sqrt(1.0 - dot(Normal.xy, Normal.xy));

    Normal = BTN * Normal;

    vec3 ambientLightColor = ambientColor;

    // ambient
    float ambientStrength = texture(texSampler[3], fragTexCoord).r;
    vec3 ambient = ambientLightColor.rgb * texture(texSampler[3], fragTexCoord).rgb * texture(texSampler[0], fragTexCoord).rgb;
  	
    // diffuse 
    //vec3 norm = normalize(normal);
    
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * texture(texSampler[0], fragTexCoord).rgb;  


    // specular
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, Normal);  
    vec3 halfwayDir = normalize(lightDir + viewDir);  

    float roughness = mix(texture(texSampler[2], fragTexCoord).r, 1, 128);
    float specular  = pow(max(dot(viewDir, reflectDir), 0.0), roughness);

    vec3 meshColorOne =  ambient;
    vec3 meshColorTwo = lightColor * (diffuse + specular);
    vec3 finalColor = meshColorOne + meshColorTwo;

    outColor = vec4(finalColor, 1.0);
}