#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	fragmentColor = texture(textureSampler, fragTexCoords);
}