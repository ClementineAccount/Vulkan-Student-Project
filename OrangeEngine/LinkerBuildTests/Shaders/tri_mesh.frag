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


layout(location = 0) out vec4 outColor;



void main() {

    vec3 albedo     =  texture(texSampler[0], fragTexCoord).rgb;
    float roughness =  texture(texSampler[2], fragTexCoord).r;
    vec3 ao        =  texture(texSampler[3], fragTexCoord).rgb;
    vec3 metallic  =  texture(texSampler[4], fragTexCoord).rgb;

    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(texSampler[1], fragTexCoord).rgb;

    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space

    // get diffuse color
    vec3 color = texture(texSampler[0], fragTexCoord).rgb;

    vec3 ambient = 0.1 * color;

    // diffuse
    vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;


    // specular
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    outColor = vec4(ambient + diffuse + specular, 1.0);

}