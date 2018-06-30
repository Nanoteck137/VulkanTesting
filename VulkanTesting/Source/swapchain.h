#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL/SDL.h>

#include "types.h"

class Renderer;

struct SwapchainSupportInfo
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;

	static SwapchainSupportInfo GetSwapchainSupportInfo(vk::SurfaceKHR surface, vk::PhysicalDevice gpuDevice);

};

class Swapchain
{
private:
	Renderer* m_Renderer;
	SDL_Window* m_Window;

	vk::SwapchainKHR m_Swapchain;
	vk::SurfaceFormatKHR m_ImageFormat;
	vk::Extent2D m_Extent;

	std::vector<vk::Image> m_Images;
	std::vector<vk::ImageView> m_ImageViews;
public:
	Swapchain(SDL_Window* window, Renderer* renderer);
	~Swapchain();

	vk::SwapchainKHR GetSwapchainHandle() const { return m_Swapchain; }
	vk::SurfaceFormatKHR GetImageFormat() const { return m_ImageFormat; }
	vk::Extent2D GetExtent() const { return m_Extent; }

	const std::vector<vk::ImageView>& GetImageViews() const { return m_ImageViews; }
	const std::vector<vk::Image>& GetImages() const { return m_Images;  }
private:
	void CreateSwapchain();
	void CreateImageViews();

	//TODO: Maybe put in a Swapchain Class or orginize this a bit
	vk::SurfaceFormatKHR GetBestSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& allFormats);
	vk::PresentModeKHR GetBestSwapPresentMode(const std::vector<vk::PresentModeKHR>& allPresentModes);
	vk::Extent2D GetBestSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
};