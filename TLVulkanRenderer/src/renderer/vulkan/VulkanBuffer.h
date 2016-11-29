#pragma once

#include <vulkan/vulkan.h>
#include "SceneUtil.h"
#include <map>

namespace VulkanBuffer 
{
	// ===================
	// BUFFER
	// ===================

	struct StorageBuffer
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	};

	// ===================
	// GEOMETRIES
	// ===================
	struct GeometryBufferOffset
	{
		std::map<EVertexAttributeType, VkDeviceSize> vertexBufferOffsets;
	};

	struct GeometryBuffer
	{
		/**
		* \brief Byte offsets for vertex attributes and resource buffers into our unified buffer
		*/
		GeometryBufferOffset bufferLayout;

		/**
		* \brief Handle to the vertex buffers
		*/
		VkBuffer vertexBuffer;

		/**
		* \brief Handle to the device memory
		*/
		VkDeviceMemory vertexBufferMemory;
	};
}
