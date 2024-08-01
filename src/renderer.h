#pragma once
#include "win32/window.h"
#include <vulkan/vulkan.h>
#include <vector>

class Renderer
{
public:
	Renderer(const Window& window);

private:
	struct Context
	{
		VkInstance instance = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr;

		VkSurfaceKHR surface = nullptr;
		VkSurfaceFormatKHR surfaceFormat =
		{
			.format = VK_FORMAT_UNDEFINED,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};

		struct Swapchain
		{
			VkSwapchainKHR chain = nullptr;
			uint32_t imageCount = 0;
			std::vector<VkImage> images = {};
		} swapchain;

		struct GPU
		{
			VkPhysicalDevice physicalDevice = nullptr;
			VkDevice logicalDevice = nullptr;
			uint32_t index = 0;
		} gpu;
	} context;

	void SetupInstance();
	void SetupLayerValidation();
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