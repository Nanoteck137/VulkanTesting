#include "swapchain.h"

#include "renderer.h"

#include <SDL/SDL_vulkan.h>

Swapchain::Swapchain(SDL_Window* window, Renderer* renderer)
	: m_Window(window), m_Renderer(renderer)
{
	CreateSwapchain();
	CreateImageViews();
}

Swapchain::~Swapchain()
{
	for (vk::ImageView& imageView : m_ImageViews)
		m_Renderer->GetDevice().destroyImageView(imageView);

	m_Renderer->GetDevice().destroySwapchainKHR(m_Swapchain);
}

void Swapchain::CreateSwapchain()
{
	SwapchainSupportInfo supportInfo = SwapchainSupportInfo::GetSwapchainSupportInfo(m_Renderer->GetSurface(), m_Renderer->GetGPUDevice());

	vk::SurfaceFormatKHR surfaceFormat = GetBestSwapSurfaceFormat(supportInfo.formats);
	vk::PresentModeKHR presentMode = GetBestSwapPresentMode(supportInfo.presentModes);
	vk::Extent2D extent = GetBestSwapExtent(supportInfo.capabilities);

	uint32 imageCount = supportInfo.capabilities.minImageCount + 1;
	if (supportInfo.capabilities.maxImageCount > 0 && imageCount > supportInfo.capabilities.maxImageCount)
	{
		imageCount = supportInfo.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapchainCreateInfo(vk::SwapchainCreateFlagsKHR(), m_Renderer->GetSurface());
	swapchainCreateInfo.setMinImageCount(imageCount);
	swapchainCreateInfo.setImageFormat(surfaceFormat.format);
	swapchainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
	swapchainCreateInfo.setImageExtent(extent);
	swapchainCreateInfo.setImageArrayLayers(1);
	swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);

	QueueFamilyIndicies indices = m_Renderer->GetQueueFamilyIndicies(m_Renderer->GetGPUDevice());
	uint32 queueFamilyIndices[] = { (uint32)indices.graphicsIndex, (uint32)indices.presentIndex };

	if (indices.graphicsIndex != indices.presentIndex)
	{
		swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
		swapchainCreateInfo.setQueueFamilyIndexCount(2);
		swapchainCreateInfo.setPQueueFamilyIndices(queueFamilyIndices);
	}
	else
	{
		swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
		swapchainCreateInfo.setQueueFamilyIndexCount(0);
		swapchainCreateInfo.setPQueueFamilyIndices(NULL);
	}

	swapchainCreateInfo.setPreTransform(supportInfo.capabilities.currentTransform);
	swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
	swapchainCreateInfo.setPresentMode(presentMode);

	m_Swapchain = m_Renderer->GetDevice().createSwapchainKHR(swapchainCreateInfo);

	m_Images = m_Renderer->GetDevice().getSwapchainImagesKHR(m_Swapchain);
	assert(m_Images.size() > 0);

	m_ImageFormat = surfaceFormat;
	m_Extent = extent;
}

void Swapchain::CreateImageViews()
{
	m_ImageViews.resize(m_Images.size());

	for (uint32 index = 0; index < m_Images.size(); index++)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), m_Images[index]);
		imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
		imageViewCreateInfo.setFormat(m_ImageFormat.format);
		imageViewCreateInfo.setComponents(vk::ComponentMapping(
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity));
		imageViewCreateInfo.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		m_ImageViews[index] = m_Renderer->GetDevice().createImageView(imageViewCreateInfo);
	}
}

SwapchainSupportInfo SwapchainSupportInfo::GetSwapchainSupportInfo(vk::SurfaceKHR surface, vk::PhysicalDevice gpuDevice)
{
	SwapchainSupportInfo result = {};

	result.capabilities = gpuDevice.getSurfaceCapabilitiesKHR(surface);
	result.formats = gpuDevice.getSurfaceFormatsKHR(surface);
	result.presentModes = gpuDevice.getSurfacePresentModesKHR(surface);

	return result;
}

vk::SurfaceFormatKHR Swapchain::GetBestSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& allFormats)
{
	if (allFormats.size() == 1 && allFormats[0].format == vk::Format::eUndefined)
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };

	for (const vk::SurfaceFormatKHR& format : allFormats)
	{
		if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			return format;
	}

	return allFormats[0];
}

vk::PresentModeKHR Swapchain::GetBestSwapPresentMode(const std::vector<vk::PresentModeKHR>& allPresentModes)
{
	vk::PresentModeKHR result = vk::PresentModeKHR::eFifo;
	for (const vk::PresentModeKHR& mode : allPresentModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)
			return mode;
		else if (mode == vk::PresentModeKHR::eImmediate)
			result = mode;
	}

	return result;
}

vk::Extent2D Swapchain::GetBestSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int32 windowWidth = 0;
		int32 windowHeight = 0;
		SDL_Vulkan_GetDrawableSize(m_Window, &windowWidth, &windowHeight);

		vk::Extent2D result = { (uint32)windowWidth, (uint32)windowHeight };

		result.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, result.width));
		result.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, result.height));

		return result;
	}
}