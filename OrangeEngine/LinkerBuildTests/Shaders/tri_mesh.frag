#version 450

//[0] = base color
//[1] = normalmap texture

layout(binding = 1) uniform sampler2D texSampler[2];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[0], fragTexCoord);
}