#version 450

layout(binding = 1) uniform sampler2D texSampler[4];

//[0] = albedo
//[1] = normalmap texture
//[2] = roughnessMap texture
//[3] = aoMap Texture


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 TangentLightPos;
layout(location = 3) in vec3 TangentViewPos;
layout(location = 4) in vec3 TangentFragPos;
layout(location = 5) in vec3 fragPos;
layout(location = 6) in vec3 lightPos;
layout(location = 7) in vec3 cameraPos;

//https://learnopengl.com/Advanced-Lighting/Normal-Mapping

//https://learnopengl.com/PBR/Lighting

// (Not a lot of time to write an implementation of my own from pure scratch right now. Hope this is okay for now)

layout(location = 0) out vec4 outColor;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}



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