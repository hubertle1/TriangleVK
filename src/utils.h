#pragma once
#include <iostream>
#include <string>
#include <vulkan/vulkan.h>

inline void Validate( const VkResult& result )
{
	if( result != VK_SUCCESS )
	{
		std::cerr << "Vulkan returned status: " << result << std::endl;
		__debugbreak();
	}
}

inline void Validate( const VkResult& result, const std::string& whatWasValidatedMessage )
{
	if( result != VK_SUCCESS )
	{
		Validate( result );
	}
	else
	{
		std::cout << "The task: '" << whatWasValidatedMessage << "' was successful." << std::endl;
	}
}