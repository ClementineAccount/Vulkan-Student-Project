#version 450

//push constants block
layout( push_constant ) uniform constants
{
	vec4 light_pos;
    vec4 light_color;
    vec4 camera_pos;
    mat4 model_matrix;
	mat4 render_matrix;
} PushConstants;

//Adapted from: https://learnopengl.com/Advanced-Lighting/Normal-Mapping

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBiTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 lightPos;
layout(location = 4) out vec3 cameraPos;
layout(location = 5) out vec3 lightColor;
layout(location = 6) out vec3 localPos;
layout(location = 7) out vec3 tangentViewPos;
layout(location = 8) out vec3 tangentLightPos;
layout(location = 9) out vec3 tangentFragPos;
layout (location = 10) out vec3 normal;


void main() {

    cameraPos  = vec3(PushConstants.camera_pos.x, PushConstants.camera_pos.y, PushConstants.camera_pos.z);
    lightPos   = vec3(PushConstants.light_pos.x, PushConstants.light_pos.y, PushConstants.light_pos.z);
    lightColor = vec3(PushConstants.light_color.x,  PushConstants.light_color.y, PushConstants.light_color.z);

    mat4 W2L = inverse(PushConstants.model_matrix);
    
    fragPos = vec3(PushConstants.model_matrix * vec4(inPosition, 1.0)); 
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    normal = inNormal;

    mat3 normalMatrix = transpose(inverse(mat3(PushConstants.model_matrix)));
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 N = normalize(normalMatrix * inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));    
    tangentLightPos = TBN * PushConstants.light_pos.xyz;
    tangentViewPos  = TBN * PushConstants.camera_pos.xyz;
    tangentFragPos  = TBN * fragPos;

    gl_Position = PushConstants.render_matrix * vec4(inPosition, 1.0);
}
