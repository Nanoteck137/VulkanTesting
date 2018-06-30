#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL/SDL.h>

#include "types.h"
#include "swapchain.h"

struct QueueFamilyIndicies
{
	int32 graphicsIndex = -1;
	int32 presentIndex = -1;

	bool IsComplete() const
	{
		return graphicsIndex >= 0 && presentIndex >= 0;
	}
};

class Renderer
{
private:
	SDL_Window* m_Window;

	VkDebugReportCallbackCreateInfoEXT m_DebugReportCallbackCreateInfo;
	vk::Instance m_Instance;
	vk::SurfaceKHR m_Surface;
	vk::DebugReportCallbackEXT m_DebugReportCallback;

	QueueFamilyIndicies m_QueueFamilyIndicies;
	vk::PhysicalDevice m_GPUDevice;
	vk::Device m_Device;

	vk::Queue m_GraphicsQueue;
	vk::Queue m_PresentQueue;

	Swapchain* m_Swapchain;

	std::vector<const char*> m_InstanceExtentions;
	std::vector<const char*> m_InstanceLayers;

	std::vector<const char*> m_DeviceExtenstions;
public:
	Renderer(SDL_Window* window);
	~Renderer();

	vk::Instance GetInstance() const { return m_Instance; }
	vk::SurfaceKHR GetSurface() const { return m_Surface; }

	vk::PhysicalDevice GetGPUDevice() const { return m_GPUDevice; }
	vk::Device GetDevice() const { return m_Device; }

	vk::Queue GetGraphicsQueue() const { return m_GraphicsQueue; }
	vk::Queue GetPresentQueue() const { return m_PresentQueue; }

	Swapchain* GetSwapchain() const { return m_Swapchain; }

	QueueFamilyIndicies GetQueueFamilyIndicies(vk::PhysicalDevice gpuDevice);
private:
	void Init();

	void CreateInstance();
	void CreateSurface();
	void SetupDebugReport();

	bool IsGPUDeviceSuitable(vk::PhysicalDevice gpuDevice);
	void CreateDevice();
};