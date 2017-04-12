#include <gli/gli.hpp>
#include <gli/texture.hpp>
#include <gli/texture_cube.hpp>
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
			VkImageCreateFlags flags,
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
			flags,
			this->image,
			this->imageMemory
		);

		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		{
			// Create image view, sampler, descriptor if we're going to use this
			VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
			if (flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
				viewType = VK_IMAGE_VIEW_TYPE_CUBE;


				// Setup buffer copy regions for each face including all of it's miplevels
				std::vector<VkBufferImageCopy> bufferCopyRegions;
				size_t offset = 0;

				for (uint32_t face = 0; face < 6; face++)
				{
					for (uint32_t level = 0; level < mipLevels; level++)
					{
						VkBufferImageCopy bufferCopyRegion = {};
						bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						bufferCopyRegion.imageSubresource.mipLevel = level;
						bufferCopyRegion.imageSubresource.baseArrayLayer = face;
						bufferCopyRegion.imageSubresource.layerCount = 1;
						bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texCube[face][level].extent().x);
						bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texCube[face][level].extent().y);
						bufferCopyRegion.imageExtent.depth = 1;
						bufferCopyRegion.bufferOffset = offset;

						bufferCopyRegions.push_back(bufferCopyRegion);

						// Increase offset into staging buffer for next level / face
						offset += texCube[face][level].size();
					}
				}
			}

			device->CreateImageView(
				this->image,
				viewType,
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
		VkImageCreateFlags createFlags,
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
				createFlags,
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
				createFlags,
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
			VkImageCreateFlags createFlags,
			VkImageLayout imageLayout,
			bool forceLinear
		)
	{
		m_device = device;
		int texWidth, texHeight, texChannels;
		Byte* imageBytes = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		gli::texture_cube Texture = gli::load(filepath);
		if (Texture.empty())
			return 0;

		gli::gl GL(gli::gl::PROFILE_GL33);
		gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
		GLenum Target = GL.translate(Texture.target());

		GLuint TextureName = 0;
		glGenTextures(1, &TextureName);
		glBindTexture(Target, TextureName);
		glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

		glm::tvec3<GLsizei> const Extent(Texture.extent());
		GLsizei const FaceTotal = static_cast<GLsizei>(Texture.layers() * Texture.faces());

		switch (Texture.target())
		{
			case gli::TARGET_1D:
				glTexStorage1D(
					Target, static_cast<GLint>(Texture.levels()), Format.Internal, Extent.x);
				break;
			case gli::TARGET_1D_ARRAY:
			case gli::TARGET_2D:
			case gli::TARGET_CUBE:
				glTexStorage2D(
					Target, static_cast<GLint>(Texture.levels()), Format.Internal,
					Extent.x, Texture.target() == gli::TARGET_2D ? Extent.y : FaceTotal);
				break;
			case gli::TARGET_2D_ARRAY:
			case gli::TARGET_3D:
			case gli::TARGET_CUBE_ARRAY:
				glTexStorage3D(
					Target, static_cast<GLint>(Texture.levels()), Format.Internal,
					Extent.x, Extent.y,
					Texture.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
				break;
			default:
				assert(0);
				break;
		}

		for (std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
			for (std::size_t Face = 0; Face < Texture.faces(); ++Face)
				for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
				{
					GLsizei const LayerGL = static_cast<GLsizei>(Layer);
					glm::tvec3<GLsizei> Extent(Texture.extent(Level));
					Target = gli::is_target_cube(Texture.target())
						? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
						: Target;

					switch (Texture.target())
					{
						case gli::TARGET_1D:
							if (gli::is_compressed(Texture.format()))
								glCompressedTexSubImage1D(
									Target, static_cast<GLint>(Level), 0, Extent.x,
									Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
									Texture.data(Layer, Face, Level));
							else
								glTexSubImage1D(
									Target, static_cast<GLint>(Level), 0, Extent.x,
									Format.External, Format.Type,
									Texture.data(Layer, Face, Level));
							break;
						case gli::TARGET_1D_ARRAY:
						case gli::TARGET_2D:
						case gli::TARGET_CUBE:
							if (gli::is_compressed(Texture.format()))
								glCompressedTexSubImage2D(
									Target, static_cast<GLint>(Level),
									0, 0,
									Extent.x,
									Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
									Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
									Texture.data(Layer, Face, Level));
							else
								glTexSubImage2D(
									Target, static_cast<GLint>(Level),
									0, 0,
									Extent.x,
									Texture.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
									Format.External, Format.Type,
									Texture.data(Layer, Face, Level));
							break;
						case gli::TARGET_2D_ARRAY:
						case gli::TARGET_3D:
						case gli::TARGET_CUBE_ARRAY:
							if (gli::is_compressed(Texture.format()))
								glCompressedTexSubImage3D(
									Target, static_cast<GLint>(Level),
									0, 0, 0,
									Extent.x, Extent.y,
									Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
									Format.Internal, static_cast<GLsizei>(Texture.size(Level)),
									Texture.data(Layer, Face, Level));
							else
								glTexSubImage3D(
									Target, static_cast<GLint>(Level),
									0, 0, 0,
									Extent.x, Extent.y,
									Texture.target() == gli::TARGET_3D ? Extent.z : LayerGL,
									Format.External, Format.Type,
									Texture.data(Layer, Face, Level));
							break;
						default: assert(0); break;
					}
				}

		// Texture bytes
		Texture* texture = new ImageTexture(imageBytes, texWidth, texHeight);
		CreateFromTexture(device, texture, format, imageUsageFlags, createFlags, imageLayout, forceLinear);

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
