#pragma once
#include "win32/window.h"
#include "utils.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
	float position[ 2 ];
};

struct VulkanContext
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
		VkQueue queue = nullptr;
	} gpu;

	struct Semaphores
	{
		VkSemaphore submit = nullptr;
		VkSemaphore acquire = nullptr;
	} semaphore;

	VkCommandPool commandPool = nullptr;
	VkRenderPass renderPass = nullptr;

	std::vector<VkImageView> imageViews = {};
	std::vector<VkFramebuffer> frameBuffers = {};

	struct PipelineInfo
	{
		VkPipelineLayout layout = nullptr;
		VkPipeline pipeline = nullptr;
	} pipelineInfo;

	struct VertexBuffer
	{
		VkBuffer buffer = nullptr;
		VkDeviceMemory memory = nullptr;
		VkDeviceSize size = 0;
	} vertexBuffer;
};

class Context
{
public:
	Context( const Window& window, const std::vector<Vertex>& vertices );
	const VulkanContext& Get() const;

private:
	VulkanContext context;

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
	std::pair<const VkPhysicalDevice&, const uint32_t&> DetectGPU() const;
	uint32_t GetGPUIndex( const VkPhysicalDevice& gpu ) const;

	void SetupSwapchain();
	VkSurfaceFormatKHR GetSurfaceFormat() const;

	void SetupCommandPool();
	void SetupSemaphores();
	void SetupRenderPass();
	void SetupImageViews();
	void SetupFrameBuffers( const Window& window );

	void SetupGraphicsPipeline();
	VkShaderModule CreateShaderModule( std::string path );
	std::pair<void*, uint32_t> ReadShaderFile( std::string path );

	void SetupVertexBuffer( const std::vector<Vertex>& vertices );
	uint32_t GetMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const;
};