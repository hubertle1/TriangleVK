#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 fragmentColor;

void main()
{
	fragmentColor = vec4(1.0, 1.0, 1.0, 1.0);
}