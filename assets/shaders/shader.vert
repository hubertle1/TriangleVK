#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(push_constant) uniform PushConstants {
    mat4 model;
} transform;

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 texCoords;

layout(location = 0) out vec2 fragTexCoords;

void main()
{
	gl_Position = transform.model * vec4(coords, 0.0, 1.0);
	fragTexCoords = texCoords;
}