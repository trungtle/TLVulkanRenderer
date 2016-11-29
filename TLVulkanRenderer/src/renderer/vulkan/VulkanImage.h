#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanImage {

	struct Image
	{
		int width;
		int height;
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory imageMemory;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;
	};

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


