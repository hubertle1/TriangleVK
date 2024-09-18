#pragma once
#include "context.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_win32.h>

#include <glm/glm.hpp>

Context::Context( const Window& window, const std::vector<Vertex>& vertices )
{
	this->SetupInstance();
	this->SetupSurface( window );
	this->SetupGPU();
	
	this->SetupSwapchain();
	this->SetupRenderPass();

	this->SetupImageViews();
	this->SetupFrameBuffers( window );

	this->SetupCommandPool();
	this->SetupSemaphores();

	this->SetupGraphicsPipeline();
	this->SetupVertexBuffer( vertices );
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

	VkDeviceCreateInfo deviceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
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
		.queueFamilyIndex = this->context.gpu.index
	};

	Validate(
		vkCreateCommandPool( this->context.gpu.logicalDevice, &poolInfo, 0, &this->context.commandPool ),
		"Create Command Pool"
	);
}

void Context::SetupSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,

	};

	Validate(
		vkCreateSemaphore( this->context.gpu.logicalDevice, &semaphoreInfo, 0, &this->context.semaphore.acquire ),
		"Create acquire semaphore"
	);
	
	Validate(
		vkCreateSemaphore( this->context.gpu.logicalDevice, &semaphoreInfo, 0, &this->context.semaphore.submit ),
		"Create submit semaphore"
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

void Context::SetupGraphicsPipeline()
{
	VkPushConstantRange pushConstantRange =
	{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof( glm::mat4 ),
	};

	VkPipelineLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	Validate(
		vkCreatePipelineLayout( this->context.gpu.logicalDevice, &layoutInfo, 0, &this->context.pipelineInfo.layout ),
		"Create layout pipeline"
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

	VkVertexInputAttributeDescription attributeDescription =
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof( Vertex, position ),
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &attributeDescription,
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
		.cullMode = VK_CULL_MODE_BACK_BIT,
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

	VkRect2D scissors =	{};
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
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data(),
	};

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
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
		"Failed to create vertex buffer"
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
		"Failed to allocate vertex buffer memory"
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

