#pragma once

#include <vulkan.h>
#include <vector>
#include "Texture.h"

class VulkanDevice;

namespace VulkanImage {

	class Image {
		public:
			int width;
			int height;
			uint32_t mipLevels;
			VkFormat format;
			VkImageAspectFlags aspectMask;
			VkImage image;
			VkImageView imageView;
			VkDeviceMemory imageMemory;
			VkSampler sampler;
			VkDescriptorImageInfo descriptor;

			Image() : width(0), height(0), mipLevels(0), format(), aspectMask(0), image(nullptr), imageView(nullptr), imageMemory(nullptr), sampler(nullptr), m_device(nullptr), m_texture(nullptr) {
		};
		
		~Image();

		void
		Create(
			VulkanDevice* device,
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags createFlags,
			VkImageAspectFlags aspectMask,
			VkMemoryPropertyFlags properties,
			bool repeat = false
		);

		void
		CreateFromTexture(
			VulkanDevice* device,
			Texture* texture,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageCreateFlags createFlags = 0,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			bool forceLinear = false
		);

		void
		CreateFromFile(
			VulkanDevice* device,
			std::string filepath,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageCreateFlags createFlags = 0,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			bool forceLinear = false
		);

		void
		Destroy() const;

		void
		CopyImage(const Image& other) const;

		void
		TransitionImageLayout(
			VkImageLayout oldLayout,
			VkImageLayout newLayout
		) const;

	private:
		VulkanDevice* m_device;
		/**
		 * \brief 
		 */
		ImageTexture* m_texture;
	};

	//struct Texture
	//{
	//	VkSampler sampler;
	//	VkImage image;
	//	VkImageLayout imageLayout;
	//	VkDeviceMemory deviceMemory;
	//	VkImageView view;
	//	uint32_t width, height;
	//	uint32_t mipLevels;
	//	uint32_t layerCount;
	//	VkDescriptorImageInfo descriptor;
	//};

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
