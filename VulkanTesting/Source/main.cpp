#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>

#include "types.h"

#include "renderer.h"

struct Vector2f
{
	float x, y;
};

struct Vector3f
{
	float x, y, z;
};

struct Vertex
{
	Vector2f pos;
	Vector3f color;
};

Vertex vertices[] = {
	{ { 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
};

struct BufferInfo
{
	byte* buffer;
	size_t size;
};

BufferInfo ReadFileToBuffer(const std::string& filename)
{
	FILE* file = NULL;
	fopen_s(&file, filename.c_str(), "rb");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	uint32 length = ftell(file);
	fseek(file, 0, SEEK_SET);

	byte* buffer = new byte[length];
	fread(buffer, 1, length, file);

	BufferInfo result;
	result.buffer = buffer;
	result.size = length;

	fclose(file);
	return result;
}

std::string ReadFileToString(const std::string& filename)
{
	FILE* file = NULL;
	fopen_s(&file, filename.c_str(), "rt");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	uint32 length = ftell(file);
	fseek(file, 0, SEEK_SET);

	std::string result(length, 0);
	fread(&result[0], 1, length, file);

	fclose(file);
	return result;
}

vk::ShaderModule CreateShader(vk::Device device, BufferInfo buffer)
{
	vk::ShaderModule result = device.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), buffer.size, (uint32*)buffer.buffer));

	return result;
}

vk::RenderPass CreateRenderPass(vk::Device device, vk::Format swapchainImageFormat)
{
	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.setFormat(swapchainImageFormat);
	colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	
	colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	
	colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

	colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachmentCount(1);
	subpass.setPColorAttachments(&colorAttachmentRef);

	vk::RenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.setAttachmentCount(1);
	renderPassCreateInfo.setPAttachments(&colorAttachment);
	renderPassCreateInfo.setSubpassCount(1);
	renderPassCreateInfo.setPSubpasses(&subpass);

	vk::RenderPass result = device.createRenderPass(renderPassCreateInfo);

	return result;
}

struct Pipeline
{
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;
};

Pipeline CreateGraphicsPipeline(vk::Device device, vk::Extent2D swapchainExtent, vk::RenderPass renderPass, vk::ShaderModule vertShader, vk::ShaderModule fragShader)
{
	vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(vk::PipelineShaderStageCreateFlags(),
															vk::ShaderStageFlagBits::eVertex,
															vertShader,
															"main",
															nullptr);

	vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(vk::PipelineShaderStageCreateFlags(),
															  vk::ShaderStageFlagBits::eFragment,
															  fragShader,
															  "main",
															  nullptr);
	vk::PipelineShaderStageCreateInfo  shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	vk::VertexInputBindingDescription vertexInputBindingDesc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

	vk::VertexInputAttributeDescription attrubuteDescriptions[] = {
		vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
		vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
	vertexInputStateInfo.setVertexBindingDescriptionCount(1);
	vertexInputStateInfo.setPVertexBindingDescriptions(&vertexInputBindingDesc);

	vertexInputStateInfo.setVertexAttributeDescriptionCount(2);
	vertexInputStateInfo.setPVertexAttributeDescriptions(attrubuteDescriptions);

	vk::PipelineInputAssemblyStateCreateInfo assemblyInputStateInfo(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, false);

	vk::Viewport viewport(0.0f, 0.0f, (float)swapchainExtent.width, (float)swapchainExtent.height, 0.0f, 1.0f);
	vk::Rect2D scissor({ 0, 0 }, swapchainExtent);

	vk::PipelineViewportStateCreateInfo viewportStateInfo(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
	rasterizationStateInfo.setDepthClampEnable(false);

	rasterizationStateInfo.setRasterizerDiscardEnable(false);
	rasterizationStateInfo.setPolygonMode(vk::PolygonMode::eFill);
	rasterizationStateInfo.setLineWidth(1.0f);
	rasterizationStateInfo.setCullMode(vk::CullModeFlagBits::eBack);
	rasterizationStateInfo.setFrontFace(vk::FrontFace::eClockwise);
	rasterizationStateInfo.setDepthBiasEnable(false);

	vk::PipelineMultisampleStateCreateInfo multisampleStateInfo = {};
	multisampleStateInfo.setSampleShadingEnable(false);
	multisampleStateInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);
	multisampleStateInfo.setMinSampleShading(1.0f);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	colorBlendAttachment.setBlendEnable(false);
	
	colorBlendAttachment.setSrcColorBlendFactor(vk::BlendFactor::eOne);
	colorBlendAttachment.setDstColorBlendFactor(vk::BlendFactor::eZero);
	colorBlendAttachment.setColorBlendOp(vk::BlendOp::eAdd);

	colorBlendAttachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
	colorBlendAttachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
	colorBlendAttachment.setAlphaBlendOp(vk::BlendOp::eAdd);
	
	vk::PipelineColorBlendStateCreateInfo colorBlendingStateInfo = {};
	colorBlendingStateInfo.setLogicOpEnable(VK_FALSE);
	colorBlendingStateInfo.setLogicOp(vk::LogicOp::eCopy);
	colorBlendingStateInfo.setAttachmentCount(1);
	colorBlendingStateInfo.setPAttachments(&colorBlendAttachment);

	colorBlendingStateInfo.setBlendConstants(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f });
	
	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport
	};

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo(vk::PipelineDynamicStateCreateFlags(), 1, dynamicStates);

	vk::PipelineLayout layout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo());

	
	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.setStageCount(2);
	graphicsPipelineCreateInfo.setPStages(shaderStages);
	
	graphicsPipelineCreateInfo.setPVertexInputState(&vertexInputStateInfo);
	graphicsPipelineCreateInfo.setPInputAssemblyState(&assemblyInputStateInfo);
	graphicsPipelineCreateInfo.setPViewportState(&viewportStateInfo);
	graphicsPipelineCreateInfo.setPRasterizationState(&rasterizationStateInfo);
	graphicsPipelineCreateInfo.setPMultisampleState(&multisampleStateInfo);
	graphicsPipelineCreateInfo.setPDepthStencilState(nullptr);
	graphicsPipelineCreateInfo.setPColorBlendState(&colorBlendingStateInfo);
	graphicsPipelineCreateInfo.setPDynamicState(nullptr);

	graphicsPipelineCreateInfo.setLayout(layout);
	graphicsPipelineCreateInfo.setRenderPass(renderPass);
	graphicsPipelineCreateInfo.setSubpass(0);

	graphicsPipelineCreateInfo.setBasePipelineHandle(nullptr);
	graphicsPipelineCreateInfo.setBasePipelineIndex(-1);

	vk::Pipeline pipeline = device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo);

	Pipeline result = {};
	result.pipeline = pipeline;
	result.layout = layout;

	return result;
}

uint32 FindMemoryType(vk::PhysicalDevice gpuDevice, uint32 typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(gpuDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

const uint32 NUM_FRAMES = 2;

#if 1
int main(int argc, char** argv)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	std::string file = ReadFileToString("Resources/shader.vert");

	auto result = compiler.CompileGlslToSpvAssembly(file, shaderc_shader_kind::shaderc_vertex_shader, "shader.vert", options);
	std::string test(result.begin(), result.end());
	printf("Error: %s\n", result.GetErrorMessage().c_str());
	printf("Result Text: %s\n", test.c_str());
	return 0;
}
#else
int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow("Hello World", 
										  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
										  1280, 720, 
										  SDL_WINDOW_VULKAN);
	assert(window != NULL);

	Renderer* renderer = new Renderer(window);

	vk::Device device = renderer->GetDevice();
	Swapchain* swapchain = renderer->GetSwapchain();

	vk::PhysicalDeviceProperties deviceProperties = renderer->GetGPUDevice().getProperties();
	printf("GPU Device Name: %s\n", deviceProperties.deviceName);

	const std::vector<vk::ImageView>& imageViews = swapchain->GetImageViews();
	const std::vector<vk::Image>& images = swapchain->GetImages();
	QueueFamilyIndicies queueIndicies = renderer->GetQueueFamilyIndicies(renderer->GetGPUDevice());

	BufferInfo vertexShaderFile = ReadFileToBuffer("Resources/vert.spv");
	vk::ShaderModule vertexShaderModule = CreateShader(device, vertexShaderFile);

	BufferInfo fragmentShaderFile = ReadFileToBuffer("Resources/frag.spv");
	vk::ShaderModule fragmentShaderModule = CreateShader(device, fragmentShaderFile);

	vk::RenderPass renderPass = CreateRenderPass(device, swapchain->GetImageFormat().format);
	Pipeline pipeline = CreateGraphicsPipeline(device, swapchain->GetExtent(), renderPass, vertexShaderModule, fragmentShaderModule);

	
	std::vector<vk::Framebuffer> framebuffers;
	framebuffers.resize(imageViews.size());

	for (uint32 index = 0; index < imageViews.size(); index++)
	{
		vk::ImageView attachments[] = {
			imageViews[index]
		};

		vk::FramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.setRenderPass(renderPass);
		framebufferCreateInfo.setAttachmentCount(1);
		framebufferCreateInfo.setPAttachments(attachments);
		framebufferCreateInfo.setWidth(swapchain->GetExtent().width);
		framebufferCreateInfo.setHeight(swapchain->GetExtent().height);
		framebufferCreateInfo.setLayers(1);

		framebuffers[index] = device.createFramebuffer(framebufferCreateInfo);
	}

	vk::BufferCreateInfo vertexBufferCreateInfo = {};
	vertexBufferCreateInfo.setSize(sizeof(Vertex) * 3);
	vertexBufferCreateInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
	vertexBufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive);

	vk::Buffer vertexBuffer = device.createBuffer(vertexBufferCreateInfo);

	vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(vertexBuffer);

	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.setAllocationSize(memRequirements.size);
	allocInfo.setMemoryTypeIndex(FindMemoryType(renderer->GetGPUDevice(), 
								 memRequirements.memoryTypeBits, 
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	vk::DeviceMemory vertexBufferMemory = device.allocateMemory(allocInfo);

	device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

	void* data = device.mapMemory(vertexBufferMemory, 0, vertexBufferCreateInfo.size);
	memcpy(data, vertices, vertexBufferCreateInfo.size);
	device.unmapMemory(vertexBufferMemory);

	vk::CommandPool commandPool = device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), queueIndicies.graphicsIndex));

	vk::CommandBufferAllocateInfo commandBufferAllocInfo(commandPool, vk::CommandBufferLevel::ePrimary, (uint32)framebuffers.size());
	std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(commandBufferAllocInfo);

	vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4> { 1.0f, 0.0f, 1.0f, 1.0f }));

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

/*	for (uint32 index = 0; index < imageViews.size(); index++)
	{
		vk::CommandBuffer commandBuffer = commandBuffers[index];
		commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr));

		vk::ImageMemoryBarrier presentToClearBarrier = {};
		presentToClearBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
		presentToClearBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		
		presentToClearBarrier.setOldLayout(vk::ImageLayout::eUndefined);
		presentToClearBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
		
		presentToClearBarrier.setSrcQueueFamilyIndex(queueIndicies.presentIndex);
		presentToClearBarrier.setDstQueueFamilyIndex(queueIndicies.presentIndex);
		
		presentToClearBarrier.setImage(images[index]);
		presentToClearBarrier.setSubresourceRange(subResourceRange);

		// Change layout of image to be optimal for presenting
		vk::ImageMemoryBarrier clearToPresentBarrier = {};
		
		clearToPresentBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		clearToPresentBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

		clearToPresentBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		clearToPresentBarrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
		
		clearToPresentBarrier.setSrcQueueFamilyIndex(queueIndicies.presentIndex);
		clearToPresentBarrier.setDstQueueFamilyIndex(queueIndicies.presentIndex);
		
		clearToPresentBarrier.setImage(images[index]);
		clearToPresentBarrier.setSubresourceRange(subResourceRange);

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, { presentToClearBarrier });
		commandBuffer.clearColorImage(images[index], vk::ImageLayout::eTransferDstOptimal, clearColor, { subResourceRange });
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, { clearToPresentBarrier });

		commandBuffer.end();
	}*/

	for (uint32 index = 0; index < imageViews.size(); index++)
	{
		vk::CommandBuffer commandBuffer = commandBuffers[index];
		commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr));

		vk::ImageMemoryBarrier presentToClearBarrier = {};
		presentToClearBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
		presentToClearBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		presentToClearBarrier.setOldLayout(vk::ImageLayout::eUndefined);
		presentToClearBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);

		presentToClearBarrier.setSrcQueueFamilyIndex(queueIndicies.presentIndex);
		presentToClearBarrier.setDstQueueFamilyIndex(queueIndicies.presentIndex);

		presentToClearBarrier.setImage(images[index]);
		presentToClearBarrier.setSubresourceRange(subResourceRange);

		// Change layout of image to be optimal for presenting
		vk::ImageMemoryBarrier clearToPresentBarrier = {};

		clearToPresentBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		clearToPresentBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

		clearToPresentBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		clearToPresentBarrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);

		clearToPresentBarrier.setSrcQueueFamilyIndex(queueIndicies.presentIndex);
		clearToPresentBarrier.setDstQueueFamilyIndex(queueIndicies.presentIndex);

		clearToPresentBarrier.setImage(images[index]);
		clearToPresentBarrier.setSubresourceRange(subResourceRange);

		vk::RenderPassBeginInfo renderPassInfo(renderPass, 
											   framebuffers[index], 
											   vk::Rect2D(vk::Offset2D(), swapchain->GetExtent()), 
											   1, &clearColor);

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);

		commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });

		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}


	std::vector<vk::Semaphore> imageAvailableSemaphore(NUM_FRAMES);
	std::vector<vk::Semaphore> renderingDoneSemaphore(NUM_FRAMES);
	std::vector<vk::Fence> fences(NUM_FRAMES);

	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

	for (uint32 index = 0; index < NUM_FRAMES; index++)
	{
		imageAvailableSemaphore[index] = device.createSemaphore(semaphoreCreateInfo);
		renderingDoneSemaphore[index] = device.createSemaphore(semaphoreCreateInfo);
		fences[index] = device.createFence(fenceCreateInfo);
	}

	uint32 currentFrame = 0;

	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				running = false;
		}

		device.waitForFences({ fences[currentFrame] }, true, UINT64_MAX);
		device.resetFences({ fences[currentFrame] });

		vk::ResultValue<uint32> imageIndex = device.acquireNextImageKHR(swapchain->GetSwapchainHandle(), 
																		UINT64_MAX, 
																		imageAvailableSemaphore[currentFrame], 
																		nullptr);
		if (imageIndex.result != vk::Result::eSuccess && 
			imageIndex.result != vk::Result::eSuboptimalKHR) 
		{
			std::cerr << "Error: Failed to acquire image" << std::endl;
			exit(1);
		}

		vk::SubmitInfo submitInfo = {};
		submitInfo.setWaitSemaphoreCount(1);
		submitInfo.setPWaitSemaphores(&imageAvailableSemaphore[currentFrame]);

		submitInfo.setSignalSemaphoreCount(1);
		submitInfo.setPSignalSemaphores(&renderingDoneSemaphore[currentFrame]);

		vk::PipelineStageFlags destStateMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo.setPWaitDstStageMask(&destStateMask);

		submitInfo.setCommandBufferCount(1);
		submitInfo.setPCommandBuffers(&commandBuffers[imageIndex.value]);

		renderer->GetPresentQueue().submit({ submitInfo }, fences[currentFrame]);

		vk::PresentInfoKHR presentInfo = {};
		presentInfo.setWaitSemaphoreCount(1);
		presentInfo.setPWaitSemaphores(&renderingDoneSemaphore[currentFrame]);

		presentInfo.setSwapchainCount(1);
		presentInfo.setPSwapchains(&swapchain->GetSwapchainHandle());
		presentInfo.setPImageIndices(&imageIndex.value);

		renderer->GetPresentQueue().presentKHR(presentInfo);

		currentFrame = (currentFrame + 1) % NUM_FRAMES;
		SDL_Delay(100);
	}

	device.waitIdle();

	device.destroyBuffer(vertexBuffer);
	device.freeMemory(vertexBufferMemory);

	for (vk::Framebuffer& framebuffer : framebuffers)
		device.destroyFramebuffer(framebuffer);

	device.destroyPipeline(pipeline.pipeline);
	device.destroyPipelineLayout(pipeline.layout);
	device.destroyRenderPass(renderPass);

	free(vertexShaderFile.buffer);
	free(fragmentShaderFile.buffer);

	device.destroyShaderModule(vertexShaderModule);
	device.destroyShaderModule(fragmentShaderModule);

	for (uint32 index = 0; index < NUM_FRAMES; index++)
	{
		device.destroySemaphore(imageAvailableSemaphore[index]);
		device.destroySemaphore(renderingDoneSemaphore[index]);
		device.destroyFence(fences[index]);
	}

	device.destroyCommandPool(commandPool);
	delete renderer;

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
#endif