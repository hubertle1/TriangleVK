#include "renderer.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_win32.h>

Renderer::Renderer( const Window& window )
{
	this->SetupInstance();
	this->SetupSurface( window );
	this->SetupGPU();
}

void Renderer::SetupInstance()
{
	VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Triangle Vulkan App",
		.apiVersion = VK_MAKE_API_VERSION( 0, 1, 3, 290 )
	};

	std::array<const char*, 2> extensions =
	{
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME
	};

	VkInstanceCreateInfo instanceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	this->Validate(
		vkCreateInstance( &instanceCreateInfo, nullptr, &this->context.instance ), 
		"Create instance"
	);
}

void Renderer::SetupSurface( const Window& window )
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window.GetModule(),
		.hwnd = window.GetWindow()
	};

	this->Validate(
		vkCreateWin32SurfaceKHR( this->context.instance, &surfaceInfo, 0, &this->context.surface ),
		"Create surface"
	);
}

void Renderer::SetupGPU()
{
	uint32_t gpusAvailable = 0;
	this->Validate(
		vkEnumeratePhysicalDevices( this->context.instance, &gpusAvailable, 0 ),
		"Get physical devices count"
	);

	std::vector<VkPhysicalDevice> gpus(gpusAvailable);
	this->Validate(
		vkEnumeratePhysicalDevices( this->context.instance, &gpusAvailable, gpus.data()),
		"Get physical devices data"
	);

	for( const auto& gpu : gpus )
	{
		auto index = this->GetGPUIndex( gpu );

		this->context.gpu.physicalDevice = gpu;
		this->context.gpu.index = index;
	}

	VkDeviceCreateInfo deviceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
	};

	this->Validate( 
		vkCreateDevice( this->context.gpu.physicalDevice, &deviceInfo, 0, &this->context.gpu.logicalDevice ),
		"Detect GPU"
	);
}

uint32_t Renderer::GetGPUIndex( const VkPhysicalDevice& gpu ) const
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( gpu, &queueFamilyCount, 0 );

	std::vector<VkQueueFamilyProperties> queueProps( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( gpu, &queueFamilyCount, queueProps.data() );

	for( uint32_t i = 0; i < queueProps.size(); ++i )
	{
		VkBool32 doesSupportSurface = VK_FALSE;
		this->Validate(
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

void Renderer::Validate( VkResult result, const std::string& whatWasValidatedMessage ) const
{
	if( result != VK_SUCCESS )
	{
		std::cerr << "Vulkan returned status: " << result << std::endl;
		__debugbreak();
	}
	else
	{
		std::cout << "The task: '" << whatWasValidatedMessage << "' was successful." << std::endl;
	}
}
