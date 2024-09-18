#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec2 coords;

void main()
{
	gl_Position = vec4(coords, 1.0, 1.0);
}