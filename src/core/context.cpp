#pragma once
#include "context.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_win32.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

Context::Context( const Window& window, const std::vector<Vertex>& vertices, const std::string& texturePath )
{
	this->SetupInstance();
	this->SetupSurface( window );
	this->SetupGPU();
	
	this->SetupSwapchain();
	this->SetupRenderPass();

	this->SetupImageViews();
	this->SetupFrameBuffers( window );

	this->SetupCommandPool();

	this->LoadTextureImage( texturePath );

	this->CreateTextureImageView();
	this->CreateTextureSampler();

	this->SetupGraphicsPipeline();

	this->CreateDescriptorPool();
	this->CreateDescriptorSets();

	this->SetupVertexBuffer( vertices );

	this->SetupCommandBuffers();
	this->SetupSyncObjects();
}

Context::~Context()
{
    vkDeviceWaitIdle(this->context.gpu.logicalDevice);

    for (size_t i = 0; i < context.imageAvailableSemaphores.size(); i++)
    {
        vkDestroySemaphore(context.gpu.logicalDevice, context.imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(context.gpu.logicalDevice, context.renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(context.gpu.logicalDevice, context.inFlightFences[i], nullptr);
    }

    for (auto framebuffer : this->context.frameBuffers)
    {
        vkDestroyFramebuffer(this->context.gpu.logicalDevice, framebuffer, nullptr);
    }

    for (auto imageView : this->context.imageViews)
    {
        vkDestroyImageView(this->context.gpu.logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(this->context.gpu.logicalDevice, this->context.swapchain.chain, nullptr);

    vkDestroyPipeline(this->context.gpu.logicalDevice, this->context.pipelineInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(this->context.gpu.logicalDevice, this->context.pipelineInfo.layout, nullptr);

    vkDestroyRenderPass(this->context.gpu.logicalDevice, this->context.renderPass, nullptr);

    vkDestroyDescriptorPool(this->context.gpu.logicalDevice, this->context.descriptor.pool, nullptr);
    vkDestroyDescriptorSetLayout(this->context.gpu.logicalDevice, this->context.descriptor.setLayout, nullptr);

    vkDestroySampler(this->context.gpu.logicalDevice, this->context.texture.sampler, nullptr);
    vkDestroyImageView(this->context.gpu.logicalDevice, this->context.texture.imageView, nullptr);

    vkDestroyImage(this->context.gpu.logicalDevice, this->context.texture.image, nullptr);
    vkFreeMemory(this->context.gpu.logicalDevice, this->context.texture.deviceMemory, nullptr);

    vkDestroyBuffer(this->context.gpu.logicalDevice, this->context.vertexBuffer.buffer, nullptr);
    vkFreeMemory(this->context.gpu.logicalDevice, this->context.vertexBuffer.memory, nullptr);

    vkDestroyCommandPool(this->context.gpu.logicalDevice, this->context.commandPool, nullptr);

    vkDestroyDevice(this->context.gpu.logicalDevice, nullptr);

    vkDestroySurfaceKHR(this->context.instance, this->context.surface, nullptr);

    if (this->context.debugMessenger != VK_NULL_HANDLE)
    {
        auto debugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->context.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (debugMessenger != nullptr)
        {
            debugMessenger(this->context.instance, this->context.debugMessenger, nullptr);
        }
    }

    vkDestroyInstance(this->context.instance, nullptr);
}

const VulkanContext& Context::Get() const
{
	return this->context;
}

void Context::SetupInstance()
{
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Triangle Vulkan App",
		.apiVersion = VK_MAKE_API_VERSION( 0, 1, 3, 290 )
	};

	std::array<const char*, 3> extensions =
	{
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,

		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	std::array<const char*, 1> layers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	VkInstanceCreateInfo instanceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>( layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	Validate(
		vkCreateInstance( &instanceCreateInfo, nullptr, &this->context.instance ), 
		"Create instance"
	);

	this->SetupLayerValidation();
}

void Context::SetupLayerValidation()
{
	auto DebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr( this->context.instance, "vkCreateDebugUtilsMessengerEXT" )
		);

	if( DebugUtilsMessenger != nullptr )
	{
		VkDebugUtilsMessengerCreateInfoEXT debugInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
			.pfnUserCallback = this->DebugCallback
		};

		DebugUtilsMessenger( this->context.instance, &debugInfo, 0, &this->context.debugMessenger );
	}
}

VkBool32 VKAPI_PTR Context::DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	std::cout << std::endl << pCallbackData->pMessage << std::endl;

	return VK_SUCCESS;
}

void Context::SetupSurface( const Window& window )
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window.GetModule(),
		.hwnd = window.GetWindow()
	};

	Validate(
		vkCreateWin32SurfaceKHR( this->context.instance, &surfaceInfo, 0, &this->context.surface ),
		"Create surface"
	);
}

void Context::SetupGPU()
{
	auto detectedGPU = this->DetectGPU();
	this->context.gpu.physicalDevice = detectedGPU.first;
	this->context.gpu.index = detectedGPU.second;

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = this->context.gpu.index,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	std::array<const char*, 1> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures( this->context.gpu.physicalDevice, &supportedFeatures );

	VkPhysicalDeviceFeatures deviceFeatures =
	{
		.samplerAnisotropy = supportedFeatures.samplerAnisotropy ? VK_TRUE : VK_FALSE,
	};

	VkDeviceCreateInfo deviceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	Validate( 
		vkCreateDevice( this->context.gpu.physicalDevice, &deviceInfo, 0, &this->context.gpu.logicalDevice ),
		"Detect GPU"
	);

	vkGetDeviceQueue( this->context.gpu.logicalDevice, this->context.gpu.index, 0, &this->context.gpu.queue );
}

std::pair<const VkPhysicalDevice&, const uint32_t&> Context::DetectGPU() const
{
	uint32_t gpusAvailable = 0;
	Validate(
		vkEnumeratePhysicalDevices( this->context.instance, &gpusAvailable, 0 ),
		"Get physical devices count"
	);

	std::vector<VkPhysicalDevice> gpus( gpusAvailable );
	Validate(
		vkEnumeratePhysicalDevices( this->context.instance, &gpusAvailable, gpus.data() ),
		"Get physical devices array"
	);

	const auto gpu = gpus.at( 0 );

	return { gpu, this->GetGPUIndex( gpu ) };	
}

uint32_t Context::GetGPUIndex( const VkPhysicalDevice& gpu ) const
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( gpu, &queueFamilyCount, 0 );

	std::vector<VkQueueFamilyProperties> queueProps( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( gpu, &queueFamilyCount, queueProps.data() );

	for( uint32_t i = 0; i < queueProps.size(); ++i )
	{
		VkBool32 doesSupportSurface = VK_FALSE;
		Validate(
			vkGetPhysicalDeviceSurfaceSupportKHR( gpu, i, this->context.surface, &doesSupportSurface ),
			"Check for GPU surface support"
		);

		if( queueProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT && doesSupportSurface )
		{
			return i;
		}
	}

	throw std::runtime_error( "GPU was not detected!" );
}

void Context::SetupSwapchain()
{
	this->context.surfaceFormat = this->GetSurfaceFormat();

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	Validate(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( this->context.gpu.physicalDevice, this->context.surface, &surfaceCapabilities ),
		"Get GPU Surface Capabilities"
	);

	auto& sc = surfaceCapabilities;
	uint32_t imageCount = sc.minImageCount + 1 > sc.maxImageCount ? sc.minImageCount : sc.minImageCount + 1;

	VkSwapchainCreateInfoKHR swapchainInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = this->context.surface,
		.minImageCount = imageCount,
		.imageFormat = this->context.surfaceFormat.format,
		.imageExtent = surfaceCapabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	};

	Validate(
		vkCreateSwapchainKHR( this->context.gpu.logicalDevice, &swapchainInfo, 0, &this->context.swapchain.chain ) ,
		"Create swapchain"
	);

	Validate(
		vkGetSwapchainImagesKHR( this->context.gpu.logicalDevice, this->context.swapchain.chain, &this->context.swapchain.imageCount, 0 ),
		"Get Swapchain images count"
	);

	this->context.swapchain.images.resize(this->context.swapchain.imageCount);
	Validate(
		vkGetSwapchainImagesKHR( 
			this->context.gpu.logicalDevice, this->context.swapchain.chain,
			&this->context.swapchain.imageCount, this->context.swapchain.images.data()
		),
		"Get Swapchain images array"
	);
}

VkSurfaceFormatKHR Context::GetSurfaceFormat() const
{
	uint32_t formatCount;
	Validate( vkGetPhysicalDeviceSurfaceFormatsKHR( this->context.gpu.physicalDevice, this->context.surface, &formatCount, 0 ),
		"Get GPU Surface Formats count"
	);

	std::vector<VkSurfaceFormatKHR> surfaceFormats( formatCount );
	Validate(
		vkGetPhysicalDeviceSurfaceFormatsKHR( this->context.gpu.physicalDevice, this->context.surface, &formatCount, surfaceFormats.data() ),
		"Get GPU Surface Formats array"
	);

	for( auto& format : surfaceFormats )
	{
		if( format.format == VK_FORMAT_B8G8R8A8_SRGB )
		{
			return format;
		}
	}

	throw std::runtime_error( "Cannot determine the surface format!" );
}

void Context::SetupCommandPool()
{
	VkCommandPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = this->context.gpu.index
	};

	Validate(
		vkCreateCommandPool( this->context.gpu.logicalDevice, &poolInfo, 0, &this->context.commandPool ),
		"Create Command Pool"
	);
}

void Context::SetupRenderPass()
{
	VkAttachmentDescription attachment =
	{
		.format = this->context.surfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	std::vector<VkAttachmentDescription> attachmentDescriptions =
	{
		attachment
	};

	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDescription =
	{
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
	};

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
		.pAttachments = attachmentDescriptions.data(),
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
	};

	Validate(
		vkCreateRenderPass( this->context.gpu.logicalDevice, &renderPassInfo, 0, &this->context.renderPass ),
		"Create render pass"
	);
}

void Context::SetupImageViews()
{
	VkImageViewCreateInfo imageViewCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = this->context.surfaceFormat.format,
		.subresourceRange = 
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	this->context.imageViews.resize( this->context.swapchain.imageCount );
	for( uint32_t i = 0; i < this->context.swapchain.imageCount; ++i )
	{
		imageViewCreateInfo.image = this->context.swapchain.images[ i ];
		Validate(vkCreateImageView( this->context.gpu.logicalDevice, &imageViewCreateInfo, 0, &this->context.imageViews[i] ));
	}
}

void Context::SetupFrameBuffers( const Window& window )
{
	const auto screenSize = window.GetScreenSize();

	VkFramebufferCreateInfo frameBufferCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = this->context.renderPass,
		.attachmentCount = 1,
		.width = screenSize.first,
		.height = screenSize.second,
		.layers = 1,
	};

	this->context.frameBuffers.resize( this->context.swapchain.imageCount );
	for( uint32_t i = 0; i < this->context.swapchain.imageCount; ++i )
	{
		frameBufferCreateInfo.pAttachments = &this->context.imageViews[ i ];
		Validate( vkCreateFramebuffer( this->context.gpu.logicalDevice, &frameBufferCreateInfo, 0, &this->context.frameBuffers[i]) );
	}
}


void Context::LoadTextureImage( const std::string& texturePath )
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load( texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
	if( !pixels )
	{
		throw std::runtime_error( "Failed to lead texture!" );
	}

	VkDeviceSize imageSize = static_cast<VkDeviceSize>( texWidth ) * texHeight * 4;
	VkBuffer buffer;
	VkDeviceMemory memory;

	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = imageSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	Validate( vkCreateBuffer( this->context.gpu.logicalDevice, &bufferInfo, nullptr, &buffer ),
		"Create buffer" );

	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements( this->context.gpu.logicalDevice, buffer, &memRequirements );

	VkMemoryAllocateInfo bufferMemoryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = GetMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ),
	};

	Validate( vkAllocateMemory( this->context.gpu.logicalDevice, &bufferMemoryInfo, nullptr, &memory ) );
	Validate( vkBindBufferMemory( this->context.gpu.logicalDevice, buffer, memory, 0 ) );

	void* data;
	vkMapMemory( this->context.gpu.logicalDevice, memory, 0, imageSize, 0, &data );
	memcpy( data, pixels, static_cast<size_t>( imageSize ) );
	vkUnmapMemory( this->context.gpu.logicalDevice, memory );

	stbi_image_free( pixels );

	VkImageCreateInfo imageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent = {
			.width = static_cast<uint32_t>(texWidth),
			.height = static_cast<uint32_t>(texHeight),
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	Validate( vkCreateImage( this->context.gpu.logicalDevice, &imageInfo, nullptr, &this->context.texture.image ),
		"Create image" );

	vkGetImageMemoryRequirements( this->context.gpu.logicalDevice, this->context.texture.image, &memRequirements );

	VkMemoryAllocateInfo imageMemoryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = GetMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ),
	};

	Validate( vkAllocateMemory( this->context.gpu.logicalDevice, &imageMemoryInfo, nullptr, &this->context.texture.deviceMemory ) );
	Validate( vkBindImageMemory( this->context.gpu.logicalDevice, this->context.texture.image, this->context.texture.deviceMemory, 0 ) );

	TransitionImageLayout( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	CopyBufferToImage( buffer, static_cast<uint32_t>( texWidth ), static_cast<uint32_t>( texHeight ) );
	TransitionImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	vkDestroyBuffer( this->context.gpu.logicalDevice, buffer, nullptr );
	vkFreeMemory( this->context.gpu.logicalDevice, memory, nullptr );
}

void Context::TransitionImageLayout( VkImageLayout newLayout )
{
	static VkImageLayout prevLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	static VkAccessFlags prevMask = 0;
	static VkPipelineStageFlags prevStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkAccessFlags newMask = 0;
	VkPipelineStageFlags newStage = 0;

	switch( newLayout )
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		newMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		newMask = VK_ACCESS_SHADER_READ_BIT;
		newStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;

	default:
		throw std::invalid_argument( "Unsupported layout transition!" );
	}

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = prevMask,
		.dstAccessMask = newMask,
		.oldLayout = prevLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = this->context.texture.image,
		.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	vkCmdPipelineBarrier( commandBuffer,
		prevStage, newStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier );

	EndSingleTimeCommands( commandBuffer );

	prevLayout = newLayout;
	prevMask = newMask;
	prevStage = newStage;
}

void Context::CopyBufferToImage( VkBuffer buffer, uint32_t width, uint32_t height )
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region = 
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = 
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 },
	};

	vkCmdCopyBufferToImage( commandBuffer, buffer, this->context.texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

	EndSingleTimeCommands( commandBuffer );
}

VkCommandBuffer Context::BeginSingleTimeCommands() const
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = this->context.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( this->context.gpu.logicalDevice, &allocInfo, &commandBuffer );

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	return commandBuffer;
}

void Context::EndSingleTimeCommands( VkCommandBuffer commandBuffer ) const
{
	vkEndCommandBuffer( commandBuffer );

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit( this->context.gpu.queue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( this->context.gpu.queue );

	vkFreeCommandBuffers( this->context.gpu.logicalDevice, this->context.commandPool, 1, &commandBuffer );
}


void Context::CreateTextureImageView()
{
	context.texture.imageView = CreateImageView( this->context.texture.image );
}

VkImageView Context::CreateImageView( VkImage image ) const
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if( vkCreateImageView( this->context.gpu.logicalDevice, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create ImageView for texture!" );
	}

	return imageView;
}

void Context::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	Validate( vkCreateSampler( this->context.gpu.logicalDevice, &samplerInfo, nullptr, &context.texture.sampler ),
		"Texture sampler" );
}


void Context::SetupGraphicsPipeline()
{
	VkDescriptorSetLayoutBinding samplerLayoutBinding =
	{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &samplerLayoutBinding,
	};

	Validate( vkCreateDescriptorSetLayout( this->context.gpu.logicalDevice, &descriptorSetLayoutInfo, nullptr, &context.descriptor.setLayout ),
		"Create descriptor set layout" );

	VkPushConstantRange pushConstantRange =
	{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof( float ) * 16 + sizeof( float ) + 12,	// It has to align to multiple of 16
	};

	VkPipelineLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &this->context.descriptor.setLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	Validate(
		vkCreatePipelineLayout( this->context.gpu.logicalDevice, &layoutInfo, 0, &this->context.pipelineInfo.layout ),
		"Create pipeline layout"
	);

	VkShaderModule vertexShader = this->CreateShaderModule( "assets/shaders/shader.vert.spv" );
	VkShaderModule fragmentShader = this->CreateShaderModule( "assets/shaders/shader.frag.spv" );

	VkPipelineShaderStageCreateInfo vertexStage =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShader,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo fragmentStage =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShader,
		.pName = "main",
	};

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages =
	{
		vertexStage, fragmentStage
	};

	VkVertexInputBindingDescription bindingDescription =
	{
		.binding = 0,
		.stride = sizeof( Vertex ),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions =
	{
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof( Vertex, position ),
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof( Vertex, texCoord ),
		},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescriptions.size() ),
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisampleState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};

	VkRect2D scissors = {};
	VkViewport viewport = {};

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissors,
	};

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() ),
		.pDynamicStates = dynamicStates.data(),
	};

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>( shaderStages.size() ),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = this->context.pipelineInfo.layout,
		.renderPass = this->context.renderPass,
	};


	Validate(
		vkCreateGraphicsPipelines( this->context.gpu.logicalDevice, 0, 1, &pipelineInfo, 0, &this->context.pipelineInfo.pipeline ),
		"Create graphics pipeline"
	);

	vkDestroyShaderModule( this->context.gpu.logicalDevice, vertexShader, nullptr );
	vkDestroyShaderModule( this->context.gpu.logicalDevice, fragmentShader, nullptr );
}

VkShaderModule Context::CreateShaderModule( std::string path )
{
	VkShaderModule shader;
	auto shaderBuffer = this->ReadShaderFile( path );

	VkShaderModuleCreateInfo shaderInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shaderBuffer.second,
		.pCode = static_cast<uint32_t*>( shaderBuffer.first ),
	};

	Validate( vkCreateShaderModule( this->context.gpu.logicalDevice, &shaderInfo, 0, &shader ) );
	delete shaderBuffer.first;

	return shader;
}

std::pair<void*, uint32_t> Context::ReadShaderFile( std::string path )
{
	const std::pair<void*, uint32_t>& failStatus = { nullptr, 0 };

	HANDLE file = CreateFile( path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	if( file == INVALID_HANDLE_VALUE )
	{
		throw std::runtime_error( "Failed to open file under: " + path );
	}

	LARGE_INTEGER size;
	if( !GetFileSizeEx( file, &size ) )
	{
		CloseHandle( file );
		throw std::runtime_error( "Failed to get file size" );
	}

	DWORD bytesRead;
	auto buffer = new char[ size.QuadPart ];

	if( !ReadFile( file, buffer, static_cast<DWORD>( size.QuadPart ), &bytesRead, 0 ) )
	{
		CloseHandle( file );
		throw std::runtime_error( "Failed to read file contents" );
	}

	CloseHandle( file );
	return { buffer , static_cast<uint32_t>( size.QuadPart ) };
}


void Context::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize =
	{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
	};

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize,
	};

	Validate( vkCreateDescriptorPool( this->context.gpu.logicalDevice, &poolInfo, nullptr, &context.descriptor.pool ),
		"Create descriptor pool" );
}

void Context::CreateDescriptorSets()
{
	if( this->context.descriptor.setLayout == nullptr )
	{
		throw std::runtime_error( "Descriptor set layout not created before allocating descriptor sets." );
	}

	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = this->context.descriptor.pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &context.descriptor.setLayout,
	};

	Validate( vkAllocateDescriptorSets( this->context.gpu.logicalDevice, &allocInfo, &context.descriptor.set ),
		"Allocate descriptor set" );

	VkDescriptorImageInfo imageInfo =
	{
		.sampler = this->context.texture.sampler,
		.imageView = this->context.texture.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkWriteDescriptorSet descriptorWrite =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = this->context.descriptor.set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo,
	};

	vkUpdateDescriptorSets( this->context.gpu.logicalDevice, 1, &descriptorWrite, 0, nullptr );
}


void Context::SetupVertexBuffer( const std::vector<Vertex>& vertices )
{
	VkDeviceSize bufferSize = sizeof( vertices[ 0 ] ) * vertices.size();

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	Validate(
		vkCreateBuffer( this->context.gpu.logicalDevice, &bufferInfo, nullptr, &this->context.vertexBuffer.buffer ),
		"Create vertex buffer"
	);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( this->context.gpu.logicalDevice, this->context.vertexBuffer.buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = GetMemoryType(
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		)
	};

	Validate(
		vkAllocateMemory( this->context.gpu.logicalDevice, &allocInfo, nullptr, &this->context.vertexBuffer.memory ),
		"Allocate vertex buffer memory"
	);

	vkBindBufferMemory( this->context.gpu.logicalDevice, this->context.vertexBuffer.buffer, this->context.vertexBuffer.memory, 0 );

	void* dataToCopy;
	vkMapMemory( this->context.gpu.logicalDevice, this->context.vertexBuffer.memory, 0, bufferSize, 0, &dataToCopy );
	memcpy( dataToCopy, vertices.data(), (size_t)bufferSize );
	vkUnmapMemory( this->context.gpu.logicalDevice, this->context.vertexBuffer.memory );
}

uint32_t Context::GetMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( this->context.gpu.physicalDevice, &memProperties );

	for( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
	{
		if( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties )
		{
			return i;
		}
	}

	throw std::runtime_error( "Failed to find suitable memory type" );
}

void Context::SetupCommandBuffers()
{
	this->context.commandBuffers.resize( this->context.swapchain.imageCount );

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->context.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>( this->context.commandBuffers.size() );

	Validate(
		vkAllocateCommandBuffers( this->context.gpu.logicalDevice, &allocInfo, this->context.commandBuffers.data() ),
		"Allocate command buffers"
	);
}

void Context::SetupSyncObjects()
{
	size_t maxFramesInFlight = context.swapchain.imageCount;

	context.imageAvailableSemaphores.resize( maxFramesInFlight );
	context.renderFinishedSemaphores.resize( maxFramesInFlight );
	context.inFlightFences.resize( maxFramesInFlight );

	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for( size_t i = 0; i < maxFramesInFlight; i++ )
	{
		Validate(
			vkCreateSemaphore( context.gpu.logicalDevice, &semaphoreInfo, nullptr, &context.imageAvailableSemaphores[ i ] ),
			"Create image available semaphore"
		);

		Validate(
			vkCreateSemaphore( context.gpu.logicalDevice, &semaphoreInfo, nullptr, &context.renderFinishedSemaphores[ i ] ),
			"Create render finished semaphore"
		);

		Validate(
			vkCreateFence( context.gpu.logicalDevice, &fenceInfo, nullptr, &context.inFlightFences[ i ] ),
			"Create fence"
		);
	}
}
