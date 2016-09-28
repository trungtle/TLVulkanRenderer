#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <string>
using namespace std;

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

private:

	VkInstance m_instance;
	VkDebugReportCallbackEXT m_debugCallback;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkQueue m_graphicsQueue;

	string m_name;
	bool m_isEnableValidationLayers;

	VkResult 
	InitVulkan();
	
	VkResult 
	SetupDebugCallback();
	
	VkResult 
	SelectPhysicalDevice();
	
	VkResult
	SetupLogicalDevice();

	void 
	Render();
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;

	bool IsComplete() {
		return graphicsFamily >= 0;
	}
};

bool 
CheckValidationLayerSupport(
	const vector<const char*>& validationLayers
	);

std::vector<const char*> 
GetRequiredExtensions(
	bool enableValidationLayers
	);


QueueFamilyIndices
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDeivce
	);


/**
 * \brief Check if this GPU is Vulkan compatible
 * \param VkPhysicalDevice to inspect
 * \return true if the GPU supports Vulkan
 */
bool IsDeviceVulkanCompatible(
	const VkPhysicalDevice& physicalDeivce
	);
