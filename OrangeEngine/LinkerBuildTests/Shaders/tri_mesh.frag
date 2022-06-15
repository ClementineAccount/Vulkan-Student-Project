#version 450

//[0] = base color
//[1] = normalmap texture

layout(binding = 1) uniform sampler2D texSampler[2];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 TangentLightPos;
layout(location = 3) in vec3 TangentViewPos;
layout(location = 4) in vec3 TangentFragPos;
layout(location = 5) in vec3 fragPos;
layout(location = 6) in vec3 lightPos;
layout(location = 7) in vec3 cameraPos;

//https://learnopengl.com/Advanced-Lighting/Normal-Mapping

layout(location = 0) out vec4 outColor;

void main() {

    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(texSampler[1], fragTexCoord).rgb;

    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space

    // get base color
    vec3 color = texture(texSampler[0], fragTexCoord).rgb;

    // ambient
    vec3 ambient = 0.1 * color;

    // diffuse
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);

    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    outColor = vec4(ambient + diffuse + specular, 1.0);
}