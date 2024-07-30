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
	} context;

	void SetupInstance();
	void SetupSurface( const Window& window );
	void Validate( VkResult result );
};