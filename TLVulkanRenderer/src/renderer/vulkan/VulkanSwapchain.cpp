#include "VulkanSwapchain.h"
#include <algorithm>
#include <cassert>

Swapchain::SwapchainSupport
Swapchain::QuerySwapchainSupport(
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
Swapchain::SelectDesiredSwapchainSurfaceFormat(
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
Swapchain::SelectDesiredSwapchainPresentMode(
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
Swapchain::SelectDesiredSwapchainExtent(
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

