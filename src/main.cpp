#pragma once
#include <iostream>
#include <vulkan/vulkan.h>

int main()
{
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Triangle Vulkan App"
	};

	VkInstanceCreateInfo instanceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

	VkInstance instance;

	auto result = vkCreateInstance( &instanceCreateInfo, nullptr, &instance );
	if( result == VK_SUCCESS )
	{
		std::cout << "Vulkan instance initialized successfully!" << std::endl;
	}

	return 0;
}
