#include "VulkanImage.h"
#include "VulkanDevice.h"
#include "VulkanCPURayTracer.h"

namespace VulkanImage {

	void
	Image::Create(
		VulkanDevice* device, 
		uint32_t width, 
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage, 
		VkImageAspectFlags aspectMask,
		VkMemoryPropertyFlags properties,
		bool repeat
		)
	{
		this->width = width;
		this->height = height;
		this->format = format;
		this->aspectMask = aspectMask;
		this->m_device = device;

		device->CreateImage(
			width,
			height,
			1, // only a 2D depth image
			VK_IMAGE_TYPE_2D, 
			format,
			tiling,
			usage,
			properties,
			this->image,
			this->imageMemory
		);

		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
			// Create image view, sampler, descriptor if we're going to use this
			device->CreateImageView(
				this->image,
				VK_IMAGE_VIEW_TYPE_2D,
				format,
				aspectMask,
				this->imageView
			);

			CreateDefaultImageSampler(device->device, &this->sampler, repeat);

			VulkanUtil::Make::SetDescriptorImageInfo(
				VK_IMAGE_LAYOUT_GENERAL,
				*this
			);
		}

	}

	void Image::Destroy() const {
		if (sampler)
		{
			vkDestroySampler(m_device->device, this->sampler, nullptr);
		}
		if (imageView)
		{
			vkDestroyImageView(m_device->device, this->imageView, nullptr);
		}
		if (image)
		{
			vkDestroyImage(m_device->device, this->image, nullptr);
		}
		if (imageMemory)
		{
			vkFreeMemory(m_device->device, this->imageMemory, nullptr);
		}
	}

	void Image::CopyImage(const Image& other) const 
	{
		m_device->CopyImage(
			this->image,
			other.image,
			this->width,
			this->height
		);
	}

	void 
	Image::TransitionImageLayout(
		VkImageLayout oldLayout, 
		VkImageLayout newLayout
		) const {
		m_device->TransitionImageLayout(
			this->image,
			this->format,
			this->aspectMask,
			oldLayout,
			newLayout
		);
	}

	Image::~Image() 
	{
		Destroy();
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
