#pragma once
#include <iostream>
#include <vulkan/vulkan.h>
#include "Window.h"

int main()
{
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Triangle Vulkan App",
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 290)
	};

	VkInstanceCreateInfo instanceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

	VkInstance instance;

	auto result = vkCreateInstance( &instanceCreateInfo, nullptr, &instance );
	if( result != VK_SUCCESS )
	{
		std::cerr << "Failed to initialize Vulkan instance!" << std::endl;
		return result;
	}
	std::cout << "Vulkan instance initialized successfully!" << std::endl;

	auto window = Window("Vulkan Triangle application");
	while( window.isOpen() )
	{
		window.OnUpdate();
	}

	return 0;
}
