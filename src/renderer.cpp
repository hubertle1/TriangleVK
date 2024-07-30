#include "renderer.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_win32.h>

Renderer::Renderer( const Window& window )
{
	this->SetupInstance();
	this->SetupSurface( window );
	
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

	auto result = vkCreateInstance( &instanceCreateInfo, nullptr, &this->context.instance );
	this->Validate( result );
}

void Renderer::SetupSurface( const Window& window )
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window.GetModule(),
		.hwnd = window.GetWindow()
	};

	this->Validate( vkCreateWin32SurfaceKHR( this->context.instance, &surfaceInfo, 0, &this->context.surface ) );
}

void Renderer::Validate( VkResult result )
{
	if( result != VK_SUCCESS )
	{
		std::cerr << "Vulkan returned status: " << result << std::endl;
		__debugbreak();
	}
}
