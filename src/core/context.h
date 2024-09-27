#pragma once
#include "win32/window.h"
#include "utils.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <glm/vec2.hpp>

struct Vertex
{
	glm::vec2 position;
	glm::vec2 texCoord;
	
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
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

	struct Texture
	{
		VkImage image = nullptr;
		VkDeviceMemory deviceMemory = nullptr;
		VkImageView imageView = nullptr;
		VkSampler sampler = nullptr;
	} texture;

	struct Descritor
	{
		VkDescriptorSetLayout setLayout = nullptr;
		VkDescriptorPool pool = nullptr;
		VkDescriptorSet set = nullptr;
	} descriptor;
};

class Context
{
public:
	Context( const Window& window, const std::vector<Vertex>& vertices, const std::string& texturePath );
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

	void LoadTextureImage( const std::string& texturePath );
	void TransitionImageLayout( VkImageLayout newLayout );
	void CopyBufferToImage( VkBuffer buffer, uint32_t width, uint32_t height );
	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands( VkCommandBuffer commandBuffer ) const;

	void CreateTextureImageView();
	VkImageView CreateImageView( VkImage image ) const;
	void CreateTextureSampler();

	void SetupGraphicsPipeline();
	VkShaderModule CreateShaderModule( std::string path );
	std::pair<void*, uint32_t> ReadShaderFile( std::string path );

	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void SetupVertexBuffer( const std::vector<Vertex>& vertices );
	uint32_t GetMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const;
};