#pragma once

#include <vulkan.h>
#include <vector>

class VulkanDevice;

namespace VulkanImage {

	class Image {
		public:
			int width;
			int height;
			VkFormat format;
			VkImage image;
			VkImageView imageView;
			VkDeviceMemory imageMemory;
			VkSampler sampler;
			VkDescriptorImageInfo descriptor;

			Image() : width(0), height(0), format(), image(nullptr), imageView(nullptr), imageMemory(nullptr), sampler(nullptr), m_device(nullptr) {
		};

		void
			Create(
				VulkanDevice* device,
				uint32_t width,
				uint32_t height,
				VkFormat format,
				VkImageTiling tiling,
				VkImageUsageFlags usage,
				VkImageAspectFlags aspectMask,
				VkMemoryPropertyFlags properties
			);

			~Image();
	private:
		VulkanDevice* m_device;
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
