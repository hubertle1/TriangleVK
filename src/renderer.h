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
		VkDebugUtilsMessengerEXT debugMessenger;

		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain;

		struct GPU
		{
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			uint32_t index;
		} gpu;
	} context;

	void SetupInstance();
	static VkBool32 VKAPI_PTR DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);
	
	void SetupSurface( const Window& window );
	void SetupGPU();
	uint32_t GetGPUIndex( const VkPhysicalDevice& gpu ) const;

	void SetupSwapchain();

	void Validate( VkResult result, const std::string& whatWasValidatedMessage ) const;
};