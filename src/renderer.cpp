#include "renderer.h"

Renderer::Renderer( const Window& window ) : context( Context( window ) )
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

	// render
	{
		VkClearColorValue color = { 0.25f, 0.25f, 1.0f, 1.0f };
		VkImageSubresourceRange range =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		};

		vkCmdClearColorImage( commandBuffer, ctx.swapchain.images.at( imageIndex ), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &color, 1, &range );
	}

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

	vkFreeCommandBuffers( ctx.gpu.logicalDevice, ctx.commandPool, 1, &commandBuffer );
}

