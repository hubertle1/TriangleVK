#pragma once
#include "core/renderer.h"

int main()
{
	const std::vector<Vertex> triangle =
	{
		{ { -0.5f,  0.5f }, { 0.0f, 1.0f } },
		{ {  0.0f, -0.5f }, { 0.5f, 0.0f } },
		{ {  0.5f,  0.5f }, { 1.0f, 1.0f } }
	};

	const auto& rotation = glm::vec3( 0.0f, 1.0f, 0.0f );

	auto window = Window( "Vulkan Triangle application" );
	auto renderer = Renderer( window, triangle, "assets/textures/brick.png" );

	while( window.IsOpen() )
	{
		window.OnUpdate();
		renderer.OnUpdate( rotation );
	}

	return 0;
}
