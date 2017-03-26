#include "VulkanImage.h"
#include "VulkanDevice.h"

namespace VulkanImage {

	Image 
	CreateVulkanImage(
		VulkanDevice* device, 
		uint32_t width, 
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties
		) 
	{
		Image image; 

		image.width = width;
		image.height = height;
		image.format = format;

		device->CreateImage(
			width,
			height,
			1, // only a 2D depth image
			VK_IMAGE_TYPE_2D, 
			format,
			tiling,
			usage,
			properties,
			image.image,
			image.imageMemory
		);

		return image;
	}

	void 
	DestroyVulkanImage(VulkanDevice* device, Image image) {
		vkDestroySampler(device->device, image.sampler, nullptr);
		vkDestroyImageView(device->device, image.imageView, nullptr);
		vkDestroyImage(device->device, image.image, nullptr);
		vkFreeMemory(device->device, image.imageMemory, nullptr);
	}

	VkFormat
	FindSupportedFormat(
		const VkPhysicalDevice& physicalDevice,
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR &&
				(props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
				(props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("Failed to find a supported format");
	}

	VkFormat
	FindDepthFormat(
		const VkPhysicalDevice& physicalDevice
	) {
		return FindSupportedFormat(
			physicalDevice,
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool
	DepthFormatHasStencilComponent(
		VkFormat format
	) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

}
