#pragma once
#include "win32/window.h"
#include <vulkan/vulkan.h>

class Renderer
{
public:
	Renderer(const Window& window);

private:
	struct Context
	{
		VkInstance instance;
		VkSurfaceKHR surface;

		struct GPU
		{
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			uint32_t index;
		} gpu;
	} context;

	void SetupInstance();
	void SetupSurface( const Window& window );
	void SetupGPU();
	uint32_t GetGPUIndex( const VkPhysicalDevice& gpu ) const;

	void Validate( VkResult result, const std::string& whatWasValidatedMessage ) const;
};