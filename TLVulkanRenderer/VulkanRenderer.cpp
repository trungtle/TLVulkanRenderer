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
	, m_swapchain(VK_NULL_HANDLE)
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

	result = CreateSwapchain();
	assert(result == VK_SUCCESS);
}


VulkanRenderer::~VulkanRenderer()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
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
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	// Grab validation layers
	if (m_isEnableValidationLayers) {
		assert(CheckValidationLayerSupport(VALIDATION_LAYERS));
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
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

	return m_physicalDevice != nullptr ? VK_SUCCESS : VK_ERROR_DEVICE_LOST;
}

VkResult 
VulkanRenderer::SetupLogicalDevice() 
{
	// @todo: should add new & interesting features later
	VkPhysicalDeviceFeatures deviceFeatures = {};

	// Create logical device info struct and populate it
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	// Create a set of unique queue families for the required queues
	m_queueFamilyIndices = FindQueueFamilyIndices(m_physicalDevice, m_surfaceKHR);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.presentFamily };
	for (auto i : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;
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
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	// Create the logical device
	VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
	
	// Grabs the first queue in the graphics queue family since we only need one graphics queue anyway
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);

	// Grabs the first queue in the present queue family since we only need one present queue anyway
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
	
	return result;
}

VkResult
VulkanRenderer::CreateSwapchain()
{
	// Query existing support
	SwapchainSupport swapchainSupport = QuerySwapchainSupport(m_physicalDevice, m_surfaceKHR);

	// Select three settings:
	// 1. Surface format
	// 2. Present mode
	// 3. Extent
	// @todo Right now the preference is embedded inside these functions. We need to expose it to a global configuration file somewhere instead.
	VkSurfaceFormatKHR surfaceFormat = SelectDesiredSwapchainSurfaceFormat(swapchainSupport.surfaceFormats);
	VkPresentModeKHR presentMode = SelectDesiredSwapchainPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = SelectDesiredSwapchainExtent(swapchainSupport.capabilities);

	// Select the number of images to be in our swapchain queue. To properly implement tripple buffering, 
	// we might need an extra image in the queue so bumb the count up. Also, create info struct
	// for swap chain requires the minimum image count
	uint32_t minImageCount = swapchainSupport.capabilities.minImageCount + 1;

	// Vulkan also specifies that a the maxImageCount could be 0 to indicate that there is no limit besides memory requirement.
	if (swapchainSupport.capabilities.maxImageCount > 0 &&
		minImageCount > swapchainSupport.capabilities.maxImageCount)
	{
		// Just need to restrict it to be something greater than the capable minImageCount but not greater than
		// maxImageCount if we're limited on maxImageCount
		minImageCount = swapchainSupport.capabilities.maxImageCount;
	}

	// Construct the swapchain create info struct
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = m_surfaceKHR;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.minImageCount = minImageCount;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.imageExtent = extent;

	// Amount of layers for each image. This is always 1 unless we're developing for stereoscopic 3D app.
	swapchainCreateInfo.imageArrayLayers = 1;

	// See ref. for a list of different image usage flags we could use
	// @ref: https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#VkImageUsageFlagBits
	// @todo: For now, we're just rendering directly to the image view as color attachment. It's possible to use
	// something like VK_IMAGE_USAGE_TRANSFER_DST_BIT to transfer to another image for post-processing before presenting.
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Swap chain is set up differently depending on whether or not 
	// our graphics queue and present queue are from the same family
	// Images will be drawn on the graphics queue then submitted to the present queue.
	assert(m_queueFamilyIndices.IsComplete());
	if (m_queueFamilyIndices.presentFamily == m_queueFamilyIndices.graphicsFamily) 
	{
		// Image is owned by a single queue family. Best performance. 
		// This is the case on most hardware.
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
		swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
	} 
	else 
	{
		// Image can be used by multiple queue families.
		const uint32_t indices[2] = {
			static_cast<uint32_t>(m_queueFamilyIndices.graphicsFamily),
			static_cast<uint32_t>(m_queueFamilyIndices.presentFamily),
		};
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = indices;
	}

	// This transform is applied to the image in the swapchain.
	// For ex. a 90 rotation. We don't want any transformation here for now.
	swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;

	// This is the blending to be applied with other windows
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	// Don't care about pixels obscured, unless we need them to compute some prediction.
	swapchainCreateInfo.clipped = VK_TRUE;

	// This is handle for a backup swapchain in case our current one is trashed and
	// we need to recover. 
	// @todo NULL for now.
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create the swap chain now!
	VkResult result = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain);
	if (result != VK_SUCCESS) 
	{
        throw std::runtime_error("Failed to create swapchain!");
	}

    // Initialize the vector of swapchain images here. 
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    // Initialize other swapchain related fields
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

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

	uint32_t extensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	
	for (uint32_t i = 0; i < extensionCount; ++i) {
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

VkSurfaceFormatKHR 
SelectDesiredSwapchainSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR> availableFormats
	) 
{
	assert(availableFormats.empty() == false);

	// List of some formats options we would like to choose from. Rank from most preferred down.
	// @todo: move this to a configuration file so we could easily configure it in the future
	std::vector<VkSurfaceFormatKHR> preferredFormats = {

		// *N.B*
		// We want to use sRGB to display to the screen, since that's the color space our eyes can perceive accurately.
		// See http://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
		// for more details.
		// For mannipulating colors, use a 32 bit unsigned normalized RGB since it's an easier format to do math with.

		{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
	};

	// Just return the first reasonable format we found
	for (const auto& format : availableFormats) {
		for (const auto& preferred : preferredFormats) {
			if (format.format == preferred.format && format.colorSpace == preferred.colorSpace) {
				return format;
			}
		}
	}

	// Couldn't find one that satisfy our preferrence, so just return the first one we found
	return availableFormats[0];
}

VkPresentModeKHR 
SelectDesiredSwapchainPresentMode(
	const std::vector<VkPresentModeKHR> availablePresentModes
	) 
{
	assert(availablePresentModes.empty() == false);

	// Maybe we should do tripple buffering here to avoid tearing & stuttering
	// @todo: This SHOULD be a configuration passed from somewhere else in a global configuration state
	bool enableTrippleBuffering = true;
	if (enableTrippleBuffering) {
		// Search for VK_PRESENT_MODE_MAILBOX_KHR. This is because we're interested in 
		// using tripple buffering.
		for (const auto& presentMode : availablePresentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
	}

	// Couldn't find one that satisfy our preferrence, so just return
	// VK_PRESENT_MODE_FIFO_KHR, since it is guaranteed to be supported by the Vulkan spec
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D 
SelectDesiredSwapchainExtent(
	const VkSurfaceCapabilitiesKHR surfaceCapabilities
	, bool useCurrentExtent
	, unsigned int desiredWidth
	, unsigned int desiredHeight
	) 
{
	// @ref From Vulkan 1.0.29 spec (with KHR extension) at
	// https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#VkSurfaceCapabilitiesKHR
	// "currentExtent is the current width and height of the surface, or the special value (0xFFFFFFFF, 0xFFFFFFFF) 
	// indicating that the surface size will be determined by the extent of a swapchain targeting the surface."
	// So we first check if this special value is set. If it is, proceed with the desiredWidth and desiredHeight
	if (surfaceCapabilities.currentExtent.width != 0xFFFFFFFF ||
		surfaceCapabilities.currentExtent.height != 0xFFFFFFFF) 
	{
		return surfaceCapabilities.currentExtent;
	}

	// Pick the extent that user prefer here
	VkExtent2D extent;
	
	// Properly select extent based on our surface capability's min and max of the extent
	uint32_t minWidth = surfaceCapabilities.minImageExtent.width;
	uint32_t maxWidth = surfaceCapabilities.maxImageExtent.width;
	uint32_t minHeight = surfaceCapabilities.minImageExtent.height;
	uint32_t maxHeight = surfaceCapabilities.maxImageExtent.height;
	extent.width = std::max(minWidth, std::min(maxWidth, static_cast<uint32_t>(desiredWidth)));
	extent.height = std::max(minHeight, std::min(maxHeight, static_cast<uint32_t>(desiredHeight)));

	return extent;
}
