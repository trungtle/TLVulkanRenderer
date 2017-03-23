#pragma once

#include <vulkan.h>
#include <vector>

class VulkanDevice;

namespace VulkanImage {

	struct Image {
		int width;
		int height;
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory imageMemory;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;
		VkFormat format;
	};

	struct Texture
	{
		VkSampler sampler;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
	};

	Image
	CreateVulkanImage(
		VulkanDevice* device,
		uint32_t width, 
		uint32_t height,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties
	);

	void
	DestroyVulkanImage(
		VulkanDevice* device,
		Image image
	);

	/**
	* \brief Find a supported format from a list of candidates
	* \param physicalDevice
	* \param candidates
	* \param tiling
	* \param features
	* \return
	*/
	VkFormat
	FindSupportedFormat(
		const VkPhysicalDevice& physicalDevice,
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	);

	/**
	* \brief Find a supported depth format for a given physical device
	* \param physicalDevice
	* \return
	*/
	VkFormat
	FindDepthFormat(
		const VkPhysicalDevice& physicalDevice
	);

	/**
	* \brief Return true if the VkFormat contains a stencil component
	* \param format
	* \return
	*/
	bool
	DepthFormatHasStencilComponent(
		VkFormat format
	);
}
