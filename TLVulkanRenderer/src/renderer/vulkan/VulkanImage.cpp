#include "VulkanImage.h"
#include "VulkanDevice.h"
#include "VulkanCPURayTracer.h"
#include "tinygltfloader/stb_image.h"

namespace VulkanImage
{

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

		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		{
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

	void 
	Image::CreateFromTexture(
		VulkanDevice* device,
		Texture* texture,
		VkFormat format,
		VkImageUsageFlags imageUsageFlags,
		VkImageLayout imageLayout,
		bool forceLinear
		) 
	{
		m_texture = reinterpret_cast<ImageTexture*>(texture);

		if (m_texture)
		{
			Image staging;

			// Stage image
			staging.Create(
				device,
				m_texture->width(),
				m_texture->height(),
				format,
				VK_IMAGE_TILING_LINEAR,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VkImageSubresource subresource = {};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource.mipLevel = 0;
			subresource.arrayLayer = 0;

			VkSubresourceLayout stagingImageLayout;
			vkGetImageSubresourceLayout(device->device, staging.image, &subresource, &stagingImageLayout);

			void* data;

			VkDeviceSize imageSize = m_texture->width() *  m_texture->width() * 4;
			vkMapMemory(device->device, staging.imageMemory, 0, imageSize, 0, &data);
			if (stagingImageLayout.rowPitch == m_texture->width() * 4)
			{
				memcpy(data, m_texture->getRawByte(), static_cast<size_t>(imageSize));
			}
			else
			{
				uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

				for (int y = 0; y < m_texture->height(); y++)
				{
					memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &m_texture->getRawByte()[y *  m_texture->width() * 4], m_texture->width() * 4);
				}
			}

			vkUnmapMemory(device->device, staging.imageMemory);

			// Prepare our texture for staging
			device->TransitionImageLayout(
				staging.image,
				format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			);

			this->Create(
				device,
				m_texture->width(),
				m_texture->height(),
				format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | imageUsageFlags,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

			device->TransitionImageLayout(
				image,
				format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			device->CopyImage(
				image,
				staging.image,
				m_texture->width(),
				m_texture->height());

			device->TransitionImageLayout(
				image,
				format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout);
		}
	}

	void
		Image::CreateFromFile(
			VulkanDevice* device,
			std::string filepath,
			VkFormat format,
			VkImageUsageFlags imageUsageFlags,
			VkImageLayout imageLayout,
			bool forceLinear
		)
	{
		m_device = device;
		int texWidth, texHeight, texChannels;
		std::string path = "scenes/textures/";
		path += filepath;
		Byte* imageBytes = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		// Texture bytes
		Texture* texture = new ImageTexture(imageBytes, texWidth, texHeight);
		CreateFromTexture(device, texture);

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
		if (m_texture) {
			delete m_texture;
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
