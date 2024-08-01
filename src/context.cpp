#include "context.h"
#include "utils.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_win32.h>

Context::Context( const Window& window )
{
	this->SetupInstance();
	this->SetupSurface( window );
	this->SetupGPU();
	this->SetupSwapchain();
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
}

std::pair<const VkPhysicalDevice&, const uint32_t&> Context::DetectGPU()
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

	this->context.swapchain.images.reserve(this->context.swapchain.imageCount);
	Validate(
		vkGetSwapchainImagesKHR( 
			this->context.gpu.logicalDevice, this->context.swapchain.chain,
			&this->context.swapchain.imageCount, this->context.swapchain.images.data()
		),
		"Get Swapchain images array"
	);
}

VkSurfaceFormatKHR Context::GetSurfaceFormat()
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
}
