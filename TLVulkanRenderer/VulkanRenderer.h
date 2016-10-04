/**
 * Several of the code structure here is referenced at:
 *  - https://vulkan-tutorial.com/ by Alexander Overvoorde
 *  - WSI Tutorial by Chris Hebert
 *  - https://github.com/SaschaWillems/Vulkan by Sascha Willems
 */

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
#include <set>
using namespace std;

/**
 * \brief 
 */
struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

/**
 * \brief 
 */
struct SwapchainSupport {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;

	bool IsComplete() const {
		return !surfaceFormats.empty() && !presentModes.empty();
	}
};

/**
 * \brief 
 */
class VulkanRenderer
{
public:
	VulkanRenderer(
		GLFWwindow* window
		);
	~VulkanRenderer();

private:

	/**
	 * \brief 
	 */
	GLFWwindow* m_window;
	
	/**
	 * \brief 
	 */
	VkInstance m_instance;
	VkDebugReportCallbackEXT m_debugCallback;
	VkSurfaceKHR m_surfaceKHR;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;


	/**
	 * \brief This doesn't have to be the same as the present queue
	 */
	VkQueue m_graphicsQueue;

	/**
	* \brief This doesn't have to be the same as the graphics queue
	*/
	VkQueue m_presentQueue;

	string m_name;
	bool m_isEnableValidationLayers;

	VkResult 
	InitVulkan();
	
	VkResult 
	SetupDebugCallback();
	
	VkResult
	CreateWindowSurface();

	VkResult
	SelectPhysicalDevice();
	
	VkResult
	SetupLogicalDevice();

	void 
	Render();

};

bool 
CheckValidationLayerSupport(
	const vector<const char*>& validationLayers
	);

std::vector<const char*> 
GetInstanceRequiredExtensions(
	bool enableValidationLayers
	);

std::vector<const char*>
GetDeviceRequiredExtensions(
	const VkPhysicalDevice& physicalDevice
	);


/**
 * \brief Check if this GPU is Vulkan compatible
 * \param VkPhysicalDevice to inspect
 * \return true if the GPU supports Vulkan
 */
bool IsDeviceVulkanCompatible(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
);

QueueFamilyIndices
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDevicece
	, const VkSurfaceKHR& surfaceKHR // For finding queue that can present image to our surface
);

SwapchainSupport
QuerySwapchainSupport(
	const VkPhysicalDevice& physicalDevice 
	, const VkSurfaceKHR& surface
	);

