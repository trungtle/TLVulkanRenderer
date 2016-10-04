#include <assert.h>
#include <iostream>

#include "VulkanRenderer.h"
#include <set>
#include <algorithm>

// This is the list of validation layers we want
const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};


VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFn(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       pUserData)
{
	std::cerr << pMessage << std::endl;
	return VK_FALSE;
}

VkResult 
CreateDebugReportCallbackEXT(
	VkInstance vkInstance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback
	) 
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(vkInstance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// ------------------

VulkanRenderer::VulkanRenderer(
	GLFWwindow* window
	)
	:
	m_window(window)
	, m_instance(VK_NULL_HANDLE) 
	, m_debugCallback(VK_NULL_HANDLE)
	, m_surfaceKHR(VK_NULL_HANDLE)
	, m_physicalDevice(VK_NULL_HANDLE)
	, m_device(VK_NULL_HANDLE)
	, m_graphicsQueue(VK_NULL_HANDLE)
	, m_presentQueue(VK_NULL_HANDLE)
	, m_name("Vulkan Application")
{
	VkResult result = InitVulkan();
	assert(result == VK_SUCCESS);

// Only enable the validation layer when running in debug mode
#ifdef NDEBUG
	m_isEnableValidationLayers = false;
#else
	m_isEnableValidationLayers = true;
#endif

	result = SetupDebugCallback();
	assert(result == VK_SUCCESS);

	result = CreateWindowSurface();
	assert(result == VK_SUCCESS);

	result = SelectPhysicalDevice();
	assert(result == VK_SUCCESS);

	result = SetupLogicalDevice();
	assert(result == VK_SUCCESS);
}


VulkanRenderer::~VulkanRenderer()
{
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surfaceKHR, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

VkResult 
VulkanRenderer::InitVulkan() 
{	
	// Create application info struct
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_name.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = m_name.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Grab extensions. This includes the KHR surface extension and debug layer if in debug mode
	std::vector<const char*> extensions = GetInstanceRequiredExtensions(m_isEnableValidationLayers);

	// Create instance info struct
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	// Grab validation layers
	if (m_isEnableValidationLayers) {
		assert(CheckValidationLayerSupport(VALIDATION_LAYERS));
		instanceCreateInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	} else {
		instanceCreateInfo.enabledLayerCount = 0;
	}

	return vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
}

VkResult 
VulkanRenderer::SetupDebugCallback() 
{
	
	// Do nothing if we have not enabled debug mode
	if (!m_isEnableValidationLayers) {
		return VK_SUCCESS;
	}

	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
	debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	// Report errors and warnings back
	debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugReportCallbackCreateInfo.pfnCallback = DebugCallbackFn;

	return CreateDebugReportCallbackEXT(m_instance, &debugReportCallbackCreateInfo, nullptr, &m_debugCallback);
}

VkResult 
VulkanRenderer::CreateWindowSurface() 
{
	VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surfaceKHR);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}

	return result;
}


VkResult 
VulkanRenderer::SelectPhysicalDevice() 
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
	
	if (physicalDeviceCount == 0) {
		throw std::runtime_error("Failed to find a GPU that supports Vulkan");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());
	
	for (auto physicalDevice : physicalDevices) {
		if (IsDeviceVulkanCompatible(physicalDevice, m_surfaceKHR)) {
			m_physicalDevice = physicalDevice;
			break;
		}
	}

	return VK_SUCCESS;
}

VkResult 
VulkanRenderer::SetupLogicalDevice() 
{
	// @todo: should add new & interesting features later
	VkPhysicalDeviceFeatures deviceFeatures = {};

	// Create logical device info struct and populate it
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	// Create a set of unique families for the required queues
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(m_physicalDevice, m_surfaceKHR);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily };
	for (auto i : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		queueCreateInfo.queueCount = 1; // Only using one queue for now. We actually don't need that many.
		float queuePriority = 1.0f; // Queue priority. Required even if we only have one queue.
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Append to queue create infos
		queueCreateInfos.push_back(queueCreateInfo);
	}

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	// Grab logical device extensions
	std::vector<const char*> enabledExtensions = GetDeviceRequiredExtensions(m_physicalDevice);
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

	// Grab validation layers
	if (m_isEnableValidationLayers) {
		assert(CheckValidationLayerSupport(VALIDATION_LAYERS));
		deviceCreateInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	// Create the logical device
	VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
	
	// Grabs the first queue in the graphics queue family since we only need one graphics queue anyway
	vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);

	// Grabs the first queue in the present queue family since we only need one present queue anyway
	vkGetDeviceQueue(m_device, queueFamilyIndices.presentFamily, 0, &m_presentQueue);
	
	return result;
}

void 
VulkanRenderer::Render() 
{

}


// ----------------
bool 
CheckValidationLayerSupport(
	const vector<const char*>& validationLayers
	) 
{

	unsigned int layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availalbeLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availalbeLayers.data());

	// Find the layer
	for (auto layerName : validationLayers) {
		bool foundLayer = false;
		for (auto layerProperty : availalbeLayers) {
			foundLayer = (strcmp(layerName, layerProperty.layerName) == 0);
			if (foundLayer) {
				break;
			}
		}

		if (!foundLayer) {
			return false;
		}
	}
	return true;
}

std::vector<const char*> 
GetInstanceRequiredExtensions(
	bool enableValidationLayers
	) 
{
	std::vector<const char*> extensions;

	unsigned int extensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	
	for (auto i = 0; i < extensionCount; ++i) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

std::vector<const char*> 
GetDeviceRequiredExtensions(
	const VkPhysicalDevice& physicalDevice
	) 
{
	const std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	for (auto& reqruiedExtension : requiredExtensions) {
		if (std::find_if(availableExtensions.begin(), availableExtensions.end(), 
		[&reqruiedExtension](const VkExtensionProperties& prop)
		{
			return strcmp(prop.extensionName, reqruiedExtension) == 0;
		}) == availableExtensions.end()) {
			// Couldn't find this extension, return an empty list
			return {};
		}
	}

	return requiredExtensions;
}

bool
IsDeviceVulkanCompatible(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR
)
{
	// Can this physical device support all the extensions we'll need (ex. swap chain)
	std::vector<const char*> requiredExtensions = GetDeviceRequiredExtensions(physicalDeivce);
	bool hasAllRequiredExtensions = requiredExtensions.size() > 0;

	// Check if we have the device properties desired
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDeivce, &deviceProperties);
	bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	// Query queue indices
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(physicalDeivce, surfaceKHR);

	// Query swapchain support
	SwapchainSupport swapchainSupport = QuerySwapchainSupport(physicalDeivce, surfaceKHR);

	return hasAllRequiredExtensions &&
		swapchainSupport.IsComplete() &&
		queueFamilyIndices.IsComplete();
}


QueueFamilyIndices 
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR
	)
{
	QueueFamilyIndices queueFamilyIndices;

	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDeivce, &queueFamilyPropertyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDeivce, &queueFamilyPropertyCount, queueFamilyProperties.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilyProperties) {
		// We need at least one graphics queue
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphicsFamily = i;
		}

		// We need at least one queue that can present image to the KHR surface.
		// This could be a different queue from our graphics queue.
		// @todo: enforce graphics queue and present queue to be the same?
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeivce, i, surfaceKHR, &presentationSupport);

		if (queueFamily.queueCount > 0 && presentationSupport) {
			queueFamilyIndices.presentFamily = i;
		}

		if (queueFamilyIndices.IsComplete()) {
			break;
		}

		++i;
	}

	return queueFamilyIndices;
}

SwapchainSupport 
QuerySwapchainSupport(
	const VkPhysicalDevice& physicalDevice
	, const VkSurfaceKHR& surface
	) 
{
	SwapchainSupport swapchainInfo;

	// Query surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainInfo.capabilities);

	// Query supported surface formats
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapchainInfo.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapchainInfo.surfaceFormats.data());
	}

	// Query supported surface present modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		swapchainInfo.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapchainInfo.presentModes.data());
	}

	return swapchainInfo;
}
