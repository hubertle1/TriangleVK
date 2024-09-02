#include "renderer.h"

Renderer::Renderer( const Window& window ) : window(window), context( Context( window ) )
{
}

void Renderer::OnUpdate()
{
	auto& ctx = this->context.Get();

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

	const auto& screenSize = this->window.GetScreenSize();
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

