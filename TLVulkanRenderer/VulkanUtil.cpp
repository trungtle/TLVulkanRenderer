#include "VulkanUtil.h"
#include <algorithm>
#include <cassert>
#include <GLFW/glfw3.h>

namespace VulkanUtil
{
	bool
		CheckValidationLayerSupport(
			const std::vector<const char*>& validationLayers
		)
	{

		unsigned int layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availalbeLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availalbeLayers.data());

		// Find the layer
		for (auto layerName : validationLayers)
		{
			bool foundLayer = false;
			for (auto layerProperty : availalbeLayers)
			{
				foundLayer = (strcmp(layerName, layerProperty.layerName) == 0);
				if (foundLayer)
				{
					break;
				}
			}

			if (!foundLayer)
			{
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

		for (uint32_t i = 0; i < extensionCount; ++i)
		{
			extensions.push_back(glfwExtensions[i]);
		}

		if (enableValidationLayers)
		{
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

		for (auto& reqruiedExtension : requiredExtensions)
		{
			if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
				[&reqruiedExtension](const VkExtensionProperties& prop)
			{
				return strcmp(prop.extensionName, reqruiedExtension) == 0;
			}) == availableExtensions.end())
			{
				// Couldn't find this extension, return an empty list
				return{};
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
			isDiscreteGPU &&
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
		for (const auto& queueFamily : queueFamilyProperties)
		{
			// We need at least one graphics queue
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamilyIndices.graphicsFamily = i;
			}

			// We need at least one queue that can present image to the KHR surface.
			// This could be a different queue from our graphics queue.
			// @todo: enforce graphics queue and present queue to be the same?
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeivce, i, surfaceKHR, &presentationSupport);

			if (queueFamily.queueCount > 0 && presentationSupport)
			{
				queueFamilyIndices.presentFamily = i;
			}

			if (queueFamilyIndices.IsComplete())
			{
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

		if (formatCount != 0)
		{
			swapchainInfo.surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapchainInfo.surfaceFormats.data());
		}

		// Query supported surface present modes
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
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
		for (const auto& format : availableFormats)
		{
			for (const auto& preferred : preferredFormats)
			{
				if (format.format == preferred.format && format.colorSpace == preferred.colorSpace)
				{
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
		if (enableTrippleBuffering)
		{
			// Search for VK_PRESENT_MODE_MAILBOX_KHR. This is because we're interested in 
			// using tripple buffering.
			for (const auto& presentMode : availablePresentModes)
			{
				if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
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

	VkFormat
		FindSupportedFormat(
			const VkPhysicalDevice& physicalDevice,
			const std::vector<VkFormat>& candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features
		)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR &&
				(props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
				(props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find a supported format");
	}

	VkFormat
		FindDepthFormat(
			const VkPhysicalDevice& physicalDevice
		)
	{
		return FindSupportedFormat(
			physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool
		DepthFormatHasStencilComponent(
			VkFormat format
		)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}
