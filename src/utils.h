#pragma once
#include <iostream>
#include <string>
#include <vulkan/vulkan.h>


void Validate( VkResult result, const std::string& whatWasValidatedMessage )
{
	if( result != VK_SUCCESS )
	{
		std::cerr << "Vulkan returned status: " << result << std::endl;
		__debugbreak();
	}
	else
	{
		std::cout << "The task: '" << whatWasValidatedMessage << "' was successful." << std::endl;
	}
}
