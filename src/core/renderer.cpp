#include "renderer.h"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer( const Window& window, const std::vector<Vertex>& vertices, const std::string& texturePath ) :
	window(window), 
	context( Context( window, vertices, texturePath ) )
{
}

void Renderer::OnUpdate( const glm::vec3& rotation )
{
	auto& ctx = this->context.Get();

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>( currentTime - startTime ).count();

	glm::mat4 model = glm::rotate(glm::mat4( 1.0f ), time * glm::radians( 90.0f ), rotation);
	glm::mat4 view = glm::translate(glm::mat4( 1.0f ), glm::vec3( 0.0f, 0.0f, -2.0f ));

	const auto& screenSize = this->window.GetScreenSize();
	const float aspectRatio = static_cast<float>( screenSize.first ) / static_cast<float>( screenSize.second );

	glm::mat4 projection = glm::perspective(
		glm::radians( 45.0f ),
		aspectRatio,
		0.1f,
		10.0f
	);

	glm::mat4 mvp = projection * view * model;

	uint32_t imageIndex = 0;
	Validate( vkAcquireNextImageKHR( ctx.gpu.logicalDevice, ctx.swapchain.chain, 0, ctx.semaphore.acquire, 0, &imageIndex ) );

	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo allocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = ctx.commandPool,
		.commandBufferCount = 1,
	};

	Validate( vkAllocateCommandBuffers( ctx.gpu.logicalDevice, &allocateInfo, &commandBuffer ) );

	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	Validate( vkBeginCommandBuffer( commandBuffer, &beginInfo ) );
	
	VkClearValue clearValue =
	{
		.color = { 0.25f, 0.25f, 1.0f, 1.0f }
	};

	VkRenderPassBeginInfo renderPassBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = ctx.renderPass,
		.framebuffer = ctx.frameBuffers[imageIndex],
		.renderArea = 
		{
			.extent = {
				screenSize.first,
				screenSize.second
			}	
		},
		.clearValueCount = 1,
		.pClearValues = &clearValue
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = {
			screenSize.first,
			screenSize.second
		}
	};

	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>( screenSize.first ),
		.height = static_cast<float>( screenSize.second ),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	vkCmdSetScissor( commandBuffer, 0, 1, &scissor );
	vkCmdSetViewport( commandBuffer, 0, 1, &viewport );

	VkBuffer vertexBuffers[] = { ctx.vertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );

	vkCmdPushConstants(
		commandBuffer,
		ctx.pipelineInfo.layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof( glm::mat4 ),
		&mvp
	);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ctx.pipelineInfo.layout,
		0,
		1,
		&ctx.descriptor.set,
		0,
		nullptr
	);

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineInfo.pipeline );
	vkCmdDraw( commandBuffer, 3, 1, 0, 0 );

	vkCmdEndRenderPass( commandBuffer );

	Validate( vkEndCommandBuffer( commandBuffer ) );

	VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &ctx.semaphore.acquire,
		.pWaitDstStageMask = &pipelineStageFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &ctx.semaphore.submit,
	};

	Validate( vkQueueSubmit( ctx.gpu.queue, 1, &submitInfo, 0 ) );

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &ctx.semaphore.submit,
		.swapchainCount = 1,
		.pSwapchains = &ctx.swapchain.chain,
		.pImageIndices = &imageIndex,
	};

	Validate( vkQueuePresentKHR( ctx.gpu.queue, &presentInfo ) );

	Validate( vkDeviceWaitIdle( ctx.gpu.logicalDevice ) );

	vkDeviceWaitIdle( ctx.gpu.logicalDevice );
	vkFreeCommandBuffers( ctx.gpu.logicalDevice, ctx.commandPool, 1, &commandBuffer );
}

