#pragma once
#include "renderer.h"

int main()
{
	auto window = Window( "Vulkan Triangle application" );
	auto renderer = Renderer( window );

	while( window.IsOpen() )
	{
		window.OnUpdate();
		renderer.OnUpdate();
	}

	return 0;
}
