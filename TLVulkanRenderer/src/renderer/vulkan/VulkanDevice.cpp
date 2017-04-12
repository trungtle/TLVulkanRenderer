#include "VulkanDevice.h"
#include <set>
#include <iostream>
#include "VulkanImage.h"
#include "VulkanUtil.h"

// ==================================
// Layers and extensions
// ==================================


// This is the list of validation layers we want
const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation",
};

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackFn(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData) {
	std::cerr << pMessage << std::endl;
	return VK_FALSE;
}

VkResult
CreateDebugReportCallbackEXT(
	VkInstance vkInstance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback
) {
	auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT"));
	if (func != nullptr) {
		return func(vkInstance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void
DestroyDebugReportCallbackEXT(
	VkInstance vkInstance,
	VkDebugReportCallbackEXT pCallback,
	const VkAllocationCallbacks* pAllocator
) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT"));
	if (func != nullptr) {
		func(vkInstance, pCallback, pAllocator);
	}
}

// ==================================
// Member functions
// ==================================

VkResult
VulkanDevice::InitializeVulkanInstance() {
	// Create application info struct
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_name.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = m_name.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Grab extensions. This includes the KHR surface extension and debug layer if in debug mode
	std::vector<const char*> extensions = GetInstanceRequiredExtensions(isEnableValidationLayers);

	// Create instance info struct
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	// Grab validation layers
	if (isEnableValidationLayers) {
		assert(CheckValidationLayerSupport(VALIDATION_LAYERS));
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		instanceCreateInfo.enabledLayerCount = 0;
	}

	return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
}

VkResult
VulkanDevice::SetupDebugCallback() {

	// Do nothing if we have not enabled debug mode
	if (!isEnableValidationLayers) {
		return VK_SUCCESS;
	}

	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
	debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	// Report errors and warnings back
	debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugReportCallbackCreateInfo.pfnCallback = DebugCallbackFn;

	return CreateDebugReportCallbackEXT(instance, &debugReportCallbackCreateInfo, nullptr, &debugCallback);
}

VkResult
VulkanDevice::CreateWindowSurface(
	GLFWwindow* window
) {
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}

	return result;
}


VkResult
VulkanDevice::SelectPhysicalDevice() {
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0) {
		throw std::runtime_error("Failed to find a GPU that supports Vulkan");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	for (auto pd : physicalDevices) {
		if (IsDeviceVulkanCompatible(pd, surfaceKHR)) {
			physicalDevice = pd;
			break;
		}
	}

	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	return physicalDevice != nullptr ? VK_SUCCESS : VK_ERROR_DEVICE_LOST;
}

VkResult
VulkanDevice::SetupLogicalDevice() {
	// Create logical device info struct and populate it
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;	
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Create a set of unique queue families for the required queues
	queueFamilyIndices = FindQueueFamilyIndices(physicalDevice, surfaceKHR);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily, queueFamilyIndices.computeFamily};
	for (auto familyIndex : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		queueCreateInfo.queueCount = 1; // Only using one queue for now. We actually don't need that many.
		float queuePriority = 1.0f; // Queue priority. Required even if we only have one queue.
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Append to queue create infos
		queueCreateInfos.push_back(queueCreateInfo);
	}

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	// Grab logical device extensions
	std::vector<const char*> enabledExtensions = GetDeviceRequiredExtensions(physicalDevice);
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

	// Grab validation layers
	if (isEnableValidationLayers) {
		assert(CheckValidationLayerSupport(VALIDATION_LAYERS));
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	// Create the logical device
	VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

	return result;
}

VkResult
VulkanDevice::PrepareSwapchain() {
	// Query existing support
	Swapchain::SwapchainSupport swapchainSupport = Swapchain::QuerySwapchainSupport(physicalDevice, surfaceKHR);

	// Select three settings:
	// 1. Surface format
	// 2. Present mode
	// 3. Extent
	// @todo Right now the preference is embedded inside these functions. We need to expose it to a global configuration file somewhere instead.
	VkSurfaceFormatKHR surfaceFormat = Swapchain::SelectDesiredSwapchainSurfaceFormat(swapchainSupport.surfaceFormats);
	VkPresentModeKHR presentMode = Swapchain::SelectDesiredSwapchainPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = Swapchain::SelectDesiredSwapchainExtent(swapchainSupport.capabilities);

	// Select the number of images to be in our swapchain queue. To properly implement tripple buffering, 
	// we might need an extra image in the queue so bumb the count up. Also, create info struct
	// for swap chain requires the minimum image count
	uint32_t minImageCount = swapchainSupport.capabilities.minImageCount + 1;

	// Vulkan also specifies that a the maxImageCount could be 0 to indicate that there is no limit besides memory requirement.
	if (swapchainSupport.capabilities.maxImageCount > 0 &&
		minImageCount > swapchainSupport.capabilities.maxImageCount) {
		// Just need to restrict it to be something greater than the capable minImageCount but not greater than
		// maxImageCount if we're limited on maxImageCount
		minImageCount = swapchainSupport.capabilities.maxImageCount;
	}

	// Construct the swapchain create info struct
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surfaceKHR;
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
	assert(queueFamilyIndices.IsComplete());
	if (queueFamilyIndices.presentFamily == queueFamilyIndices.graphicsFamily) {
		// Image is owned by a single queue family. Best performance. 
		// This is the case on most hardware.
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
		swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	else {
		// Image can be used by multiple queue families.
		const uint32_t indices[2] = {
			static_cast<uint32_t>(queueFamilyIndices.graphicsFamily),
			static_cast<uint32_t>(queueFamilyIndices.presentFamily),
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
	VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_swapchain.swapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}

	// Initialize the vector of swapchain images here. 
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(device, m_swapchain.swapchain, &imageCount, nullptr);
	m_swapchain.images.resize(imageCount);
	vkGetSwapchainImagesKHR(device, m_swapchain.swapchain, &imageCount, m_swapchain.images.data());

	// Initialize other swapchain related fields
	m_swapchain.imageFormat = surfaceFormat.format;
	m_swapchain.extent = extent;
	m_swapchain.aspectRatio = (float) extent.width / (float) extent.height;

	return result;
}

VkResult
VulkanDevice::PrepareDepthResources(
	const VkQueue& queue,
	const VkCommandPool& commandPool
) {
	VkFormat depthFormat = VulkanImage::FindDepthFormat(physicalDevice);

	CreateImage(
		m_swapchain.extent.width,
		m_swapchain.extent.height,
		1, // only a 2D depth image
		VK_IMAGE_TYPE_2D,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0,
		m_depthTexture.image,
		m_depthTexture.imageMemory
	);
	CreateImageView(
		m_depthTexture.image,
		VK_IMAGE_VIEW_TYPE_2D,
		depthFormat,
		1,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_depthTexture.imageView
	);

	TransitionImageLayout(
		m_depthTexture.image,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);

	return VK_SUCCESS;
}

VkResult
VulkanDevice::PrepareFramebuffers(
	const VkRenderPass& renderpass
) {
	VkResult result = VK_SUCCESS;

	m_swapchain.imageViews.resize(m_swapchain.images.size());
	for (auto i = 0; i < m_swapchain.imageViews.size(); ++i) {
		CreateImageView(
			m_swapchain.images[i],
			VK_IMAGE_VIEW_TYPE_2D,
			m_swapchain.imageFormat,
			1,
			VK_IMAGE_ASPECT_COLOR_BIT,
			m_swapchain.imageViews[i]
		);
	}

	m_swapchain.framebuffers.resize(m_swapchain.imageViews.size());

	// Attach image views to framebuffers
	for (int i = 0; i < m_swapchain.imageViews.size(); ++i) {
		std::array<VkImageView, 2> imageViews = {m_swapchain.imageViews[i], m_depthTexture.imageView};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderpass;
		framebufferCreateInfo.attachmentCount = imageViews.size();
		framebufferCreateInfo.pAttachments = imageViews.data();
		framebufferCreateInfo.width = m_swapchain.extent.width;
		framebufferCreateInfo.height = m_swapchain.extent.height;
		framebufferCreateInfo.layers = 1;

		result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_swapchain.framebuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
			return result;
		}
	}

	return result;
}

// =====================

void VulkanDevice::Initialize(GLFWwindow* window) {
	VkResult result = InitializeVulkanInstance();

	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Initalizes Vulkan instance");

	// Only enable the validation layer when running in debug mode
#ifdef NDEBUG
	isEnableValidationLayers = false;
#else
	isEnableValidationLayers = true;
#endif

	result = SetupDebugCallback();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Setup debug callback");

	result = CreateWindowSurface(window);
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created window surface");

	result = SelectPhysicalDevice();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Selected physical device");

	result = SetupLogicalDevice();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Setup logical device");

	result = PrepareSwapchain();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created swapchain");

	// Command pool for misc. usage (etc CopyBuffer)
	VkCommandPoolCreateInfo cmdPoolCreateInfo = VulkanUtil::Make::MakeCommandPoolCreateInfo(this->queueFamilyIndices.graphicsFamily);

	result = vkCreateCommandPool(this->device, &cmdPoolCreateInfo, nullptr, &m_graphicsDeviceQueue.cmdPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool.");
	}

	// Get device queue for compute
	vkGetDeviceQueue(this->device, this->queueFamilyIndices.graphicsFamily, 0, &m_graphicsDeviceQueue.queue);

	// Command pool for misc. usage (etc CopyBuffer)
	cmdPoolCreateInfo = VulkanUtil::Make::MakeCommandPoolCreateInfo(this->queueFamilyIndices.computeFamily);

	result = vkCreateCommandPool(this->device, &cmdPoolCreateInfo, nullptr, &m_computeDeviceQueue.cmdPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool.");
	}

	// Get device queue for compute
	vkGetDeviceQueue(this->device, this->queueFamilyIndices.computeFamily, 0, &m_computeDeviceQueue.queue);

}

void
VulkanDevice::Destroy() {
	vkDestroyImageView(device, m_depthTexture.imageView, nullptr);
	vkDestroyImage(device, m_depthTexture.image, nullptr);
	vkFreeMemory(device, m_depthTexture.imageMemory, nullptr);

	vkDestroySwapchainKHR(device, m_swapchain.swapchain, nullptr);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surfaceKHR, nullptr);

	DestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);

	vkDestroyInstance(instance, nullptr);
}

// ==================================
// Class functions
// ==================================

bool
VulkanDevice::IsDeviceVulkanCompatible(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR
) {
	// Can this physical device support all the extensions we'll need (ex. swap chain)
	std::vector<const char*> requiredExtensions = GetDeviceRequiredExtensions(physicalDeivce);
	bool hasAllRequiredExtensions = requiredExtensions.size() > 0;

	// Check if we have the device properties desired
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDeivce, &deviceProperties);
	bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	// Query queue indices
	VulkanDevice::QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(physicalDeivce, surfaceKHR);

	// Query swapchain support
	Swapchain::SwapchainSupport swapchainSupport = Swapchain::QuerySwapchainSupport(physicalDeivce, surfaceKHR);

	return hasAllRequiredExtensions &&
			isDiscreteGPU &&
			swapchainSupport.IsComplete() &&
			queueFamilyIndices.IsComplete();
}

// ==================================
// Helpers
// ==================================

bool
CheckValidationLayerSupport(
	const std::vector<const char*>& validationLayers
) {

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
) {
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
) {
	const std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	for (auto& reqruiedExtension : requiredExtensions) {
		if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
		                 [&reqruiedExtension](const VkExtensionProperties& prop) {
			                 return strcmp(prop.extensionName, reqruiedExtension) == 0;
		                 }) == availableExtensions.end()) {
			// Couldn't find this extension, return an empty list
			return {};
		}
	}

	return requiredExtensions;
}

VulkanDevice::QueueFamilyIndices
FindQueueFamilyIndices(
	const VkPhysicalDevice& physicalDeivce
	, const VkSurfaceKHR& surfaceKHR
) {
	VulkanDevice::QueueFamilyIndices queueFamilyIndices;

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

		// Query compute queue family
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			queueFamilyIndices.computeFamily = i;
		}

		// Query memory transfer family
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
			queueFamilyIndices.transferFamily = i;
		}

		if (queueFamilyIndices.IsComplete()) {
			break;
		}

		++i;
	}

	return queueFamilyIndices;
}

uint32_t
VulkanDevice::GetMemoryType(
	uint32_t typeFilter
	, VkMemoryPropertyFlags propertyFlags
) const {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (auto i = 0; i < memProperties.memoryTypeCount; ++i) {
		// Loop through each memory type and find a match
		if (typeFilter & (1 << i)) {
			if (memProperties.memoryTypes[i].propertyFlags & propertyFlags) {
				return i;
			}
		}
	}

	throw std::runtime_error("Failed to find a suitable memory type");
}

void
VulkanDevice::CreateBuffer(
	const VkDeviceSize size,
	const VkBufferUsageFlags usage,
	VkBuffer& buffer
) const {
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;

	// For multipurpose buffers, OR the VK_BUFFER_USAGE_ bits together
	bufferCreateInfo.usage = usage;

	// Buffer is used exclusively by the graphics queue
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VulkanUtil::CheckVulkanResult(
		vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer),
		"Failed to create buffer"
	);
}

void
VulkanDevice::CreateMemory(
	const VkMemoryPropertyFlags memoryProperties,
	const VkBuffer& buffer,
	VkDeviceMemory& memory
) const {
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;

	// *N.B*
	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT means memory allocated  can be mapped for host access using vkMapMemory
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ensures mapped memory matches allocated memory. 
	// Does not require flushing and invalidate cache before reading from mapped memory
	// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT uses the device local memorycr
	memoryAllocInfo.memoryTypeIndex =
			GetMemoryType(memoryRequirements.memoryTypeBits,
			              memoryProperties);

	VkResult result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for buffer");
	}
}

void
VulkanDevice::MapMemory(
	void* data,
	VkDeviceMemory& memory,
	VkDeviceSize size,
	VkDeviceSize offset
) {
	void* temp;
	vkMapMemory(device, memory, offset, size, 0, &temp);
	memcpy(temp, data, size);
	vkUnmapMemory(device, memory);
}

void
VulkanDevice::CreateBufferAndMemory(
	const VkDeviceSize size,
	const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags memoryProperties,
	VkBuffer& buffer,
	VkDeviceMemory& memory
) const {
	CreateBuffer(size, usage, buffer);
	CreateMemory(memoryProperties, buffer, memory);
	VkDeviceSize memoryOffset = 0;
	vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

void
VulkanDevice::CopyBuffer(
	VulkanBuffer::StorageBuffer& dstBuffer,
	VulkanBuffer::StorageBuffer& srcBuffer,
	VkDeviceSize size,
	bool isCompute
) const {
	VkCommandPool cmdPool = isCompute ? m_computeDeviceQueue.cmdPool : m_graphicsDeviceQueue.cmdPool;
	VkQueue queue = isCompute ? m_computeDeviceQueue.queue : m_graphicsDeviceQueue.queue;

	VkCommandBuffer copyCommandBuffer = BeginSingleTimeCommands(cmdPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(copyCommandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);

	EndSingleTimeCommands(queue, cmdPool, copyCommandBuffer);
}

void
VulkanDevice::CreateImage(
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	VkImageType imageType,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memPropertyFlags,
	VkImageCreateFlags flags,
	VkImage& image,
	VkDeviceMemory& imageMemory
) {
	VkImageCreateInfo imageInfo = VulkanUtil::Make::MakeImageCreateInfo(
		width,
		height,
		depth,
		imageType,
		format,
		tiling,
		usage,
		flags
	);

	VulkanUtil::CheckVulkanResult(
		vkCreateImage(device, &imageInfo, nullptr, &image),
		"Failed to create image"
	);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = GetMemoryType(
		memRequirements.memoryTypeBits,
		memPropertyFlags
	);

	VulkanUtil::CheckVulkanResult(
		vkAllocateMemory(device, &memoryAllocInfo, nullptr, &imageMemory),
		"Failed to allocate memory for image"
	);

	VulkanUtil::CheckVulkanResult(
		vkBindImageMemory(device, image, imageMemory, 0),
		"Failed to bind image memory"
	);
}

void
VulkanDevice::CreateImageView(
	const VkImage& image,
	VkImageViewType viewType,
	VkFormat format,
	uint32_t mipLevels,
	VkImageAspectFlags aspectFlags,
	VkImageView& imageView
) const {
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.viewType = viewType; // 1D, 2D, 3D textures or cubemap

	// Use default mapping for swizzle
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	// The subresourcerange field is used to specify the purpose of this image view
	// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkImageSubresourceRange
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags; // Use as color, depth, or stencil targets
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1; // Could have more if we're doing stereoscopic rendering
	if (viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
	{
		imageViewCreateInfo.subresourceRange.layerCount = 6; // Cubemap
	}

	VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkImageView");
	}
}

void
VulkanDevice::TransitionImageLayout(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldLayout,
	VkImageLayout newLayout
) const {
	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = aspectMask;
	//if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
	//	if (VulkanImage::DepthFormatHasStencilComponent(format)) {
	//		imageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	//	}
	//}
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	TransitionImageLayout(
		image,
		format,
		aspectMask,
		oldLayout,
		newLayout,
		subresourceRange
	);
}

void 
VulkanDevice::TransitionImageLayout(
	VkImage image, 
	VkFormat format, 
	VkImageAspectFlags aspectMask, 
	VkImageLayout oldLayout, 
	VkImageLayout newLayout, 
	VkImageSubresourceRange subresourceRange
	) const 
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_graphicsDeviceQueue.cmdPool);

	// Using image memory barrier to transition for image layout. 
	// There is a buffer memory barrier equivalent
	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // (this isn't the default value so we must set it)
	imageBarrier.image = image;
	imageBarrier.subresourceRange = subresourceRange;

	// *Code from Sascha Willems's Vulkan Samples:
	// https://github.com/SaschaWillems/Vulkan

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from and writes to the image have been finished
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageBarrier.dstAccessMask = imageBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageBarrier.srcAccessMask == 0)
			{
				imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
	}

	// A pipeline barrier inserts an execution dependency and 
	// a set of memory dependencies between a set of commands earlier 
	// in the command buffer and a set of commands later in the command buffer. 
	// \ref https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdPipelineBarrier
	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Happen immediately on the pipeline
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imageBarrier
	);

	EndSingleTimeCommands(m_graphicsDeviceQueue.queue, m_graphicsDeviceQueue.cmdPool, commandBuffer);
}

void
VulkanDevice::CopyImage(
	VkImage dstImage,
	VkImage srcImage,
	uint32_t width,
	uint32_t height
) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_graphicsDeviceQueue.cmdPool);

	// Subresource is sort of like a buffer for images
	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = {0, 0};
	region.dstOffset = {0, 0};
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	EndSingleTimeCommands(m_graphicsDeviceQueue.queue, m_graphicsDeviceQueue.cmdPool, commandBuffer);
}

void 
VulkanDevice::CopyBufferToImage(
	VkImage dstImage, 
	VkBuffer srcBuffer, 
	const std::vector<VkBufferImageCopy>& copyRegions
)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_graphicsDeviceQueue.cmdPool);

	// Subresource is sort of like a buffer for images
	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = copyRegions.size();

	vkCmdCopyBufferToImage(
		commandBuffer,
		srcBuffer,
		dstImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		copyRegions.size(),
		copyRegions.data()
	);

	EndSingleTimeCommands(m_graphicsDeviceQueue.queue, m_graphicsDeviceQueue.cmdPool, commandBuffer);
}

VkCommandBuffer
VulkanDevice::BeginSingleTimeCommands(
	VkCommandPool commandPool
) const {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void
VulkanDevice::EndSingleTimeCommands(
	VkQueue queue,
	VkCommandPool commandPool,
	VkCommandBuffer commandBuffer
) const {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
