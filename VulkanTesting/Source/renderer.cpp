#include "renderer.h"

#include <algorithm>
#include <iostream>
#include <set>

#include <SDL/SDL_vulkan.h>

PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       pUserData)
{
	if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		std::cerr << "Error: " << pMessage << std::endl;
 		__debugbreak();
	}
	else
	{
		std::cout << pMessage << std::endl;
	}

	return VK_FALSE;
}


Renderer::Renderer(SDL_Window* window)
	: m_Window(window)
{
	Init();
	CreateInstance();
	SetupDebugReport();
	CreateSurface();
	CreateDevice();

	m_Swapchain = new Swapchain(window, this);
}

Renderer::~Renderer()
{
	delete m_Swapchain;

	m_Device.destroy();

	DestroyDebugReportCallback(m_Instance, m_DebugReportCallback, NULL);
	m_Instance.destroy();
}

void Renderer::Init()
{
	uint32 SDLInstanceExtenstionsCount;
	SDL_Vulkan_GetInstanceExtensions(m_Window, &SDLInstanceExtenstionsCount, NULL);

	m_InstanceExtentions.resize(SDLInstanceExtenstionsCount);
	SDL_Vulkan_GetInstanceExtensions(m_Window, &SDLInstanceExtenstionsCount, m_InstanceExtentions.data());

	m_InstanceExtentions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	m_InstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	m_DeviceExtenstions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	//TODO: Check instance layers and extentions for validations i.e vk::enumareateInstanceExtenstions(); and vk::enumareateInstanceLayers();
	
	m_DebugReportCallbackCreateInfo = {};
	m_DebugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

	m_DebugReportCallbackCreateInfo.flags =
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	m_DebugReportCallbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)MyDebugReportCallback;
}

void Renderer::CreateInstance()
{
	vk::ApplicationInfo appInfo("Hello World", 0,
								"No Engine", 0,
								VK_API_VERSION_1_1);

	vk::InstanceCreateInfo instanceCreateInfo(vk::InstanceCreateFlags(), &appInfo,
											(uint32)m_InstanceLayers.size(), m_InstanceLayers.data(),
											(uint32)m_InstanceExtentions.size(), m_InstanceExtentions.data());

	instanceCreateInfo.pNext = &m_DebugReportCallbackCreateInfo;

	m_Instance = vk::createInstance(instanceCreateInfo);
}

void Renderer::CreateSurface()
{
	VkSurfaceKHR SDLSurface;
	if (!SDL_Vulkan_CreateSurface(m_Window, m_Instance, &SDLSurface))
		assert(0);

	m_Surface = vk::SurfaceKHR(SDLSurface);
}

void Renderer::SetupDebugReport()
{
	CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)m_Instance.getProcAddr("vkCreateDebugReportCallbackEXT");
	DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)m_Instance.getProcAddr("vkDestroyDebugReportCallbackEXT");

	VkDebugReportCallbackEXT debugReportCallback;
	VkResult vulkanResult = CreateDebugReportCallback(m_Instance, &m_DebugReportCallbackCreateInfo, NULL, &debugReportCallback);
	assert(vulkanResult == VK_SUCCESS);

	m_DebugReportCallback = vk::DebugReportCallbackEXT(debugReportCallback);
}

QueueFamilyIndicies Renderer::GetQueueFamilyIndicies(vk::PhysicalDevice gpuDevice)
{
	QueueFamilyIndicies result;
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = gpuDevice.getQueueFamilyProperties();

	for (int32 index = 0; index < queueFamilyProperties.size(); index++)
	{
		if (queueFamilyProperties[index].queueCount > 0 && queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			result.graphicsIndex = index;
		}

		uint32 presentSupport = gpuDevice.getSurfaceSupportKHR(index, m_Surface);
		if (queueFamilyProperties[index].queueCount > 0 && presentSupport)
		{
			result.presentIndex = index;
		}

		if (result.IsComplete())
			break;
	}

	assert(result.IsComplete());
	return result;
}


bool Renderer::IsGPUDeviceSuitable(vk::PhysicalDevice gpuDevice)
{
	QueueFamilyIndicies familyIndices = GetQueueFamilyIndicies(gpuDevice);

	//TODO: Check for swapchain extentions is present 
	SwapchainSupportInfo swapChainInfo = SwapchainSupportInfo::GetSwapchainSupportInfo(m_Surface, gpuDevice);
	bool swapChainComplete = !swapChainInfo.formats.empty() && !swapChainInfo.presentModes.empty();

	return familyIndices.IsComplete() && swapChainComplete;
}

void Renderer::CreateDevice()
{
	//TODO: Need to pick the GPU based on score insteed of taking the first one
	std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
	m_GPUDevice = physicalDevices[0];

	//TODO: Check all the physical devices
	assert(IsGPUDeviceSuitable(m_GPUDevice));

	m_QueueFamilyIndicies = GetQueueFamilyIndicies(m_GPUDevice);

	vk::PhysicalDeviceFeatures features = {};

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { m_QueueFamilyIndicies.graphicsIndex, m_QueueFamilyIndicies.presentIndex };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}


	vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), 
										  (uint32)queueCreateInfos.size(), queueCreateInfos.data(),
										  (uint32)m_InstanceLayers.size(), m_InstanceLayers.data(), 
										  (uint32)m_DeviceExtenstions.size(), m_DeviceExtenstions.data(), 
										  &features);

	m_Device = m_GPUDevice.createDevice(deviceCreateInfo);

	m_GraphicsQueue = m_Device.getQueue(m_QueueFamilyIndicies.graphicsIndex, 0);
	m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndicies.presentIndex, 0);
}





