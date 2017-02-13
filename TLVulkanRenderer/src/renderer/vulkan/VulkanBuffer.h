#pragma once

#include "VulkanSDK/1.0.33.0/Include/vulkan.h"
#include "scene/SceneUtil.h"
#include <map>

namespace VulkanBuffer {
	// ===================
	// BUFFER
	// ===================

	struct StorageBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
		VulkanDevice* device;

		void Create(
			const VulkanDevice* device,
			const VkDeviceSize size,
			const VkBufferUsageFlags usage,
			const VkMemoryPropertyFlags memoryProperties
		) {

			device->CreateBufferAndMemory(
				size,
				usage,
				memoryProperties,
				buffer,
				memory
			);

			descriptor.buffer = buffer;
			descriptor.offset = 0;
			descriptor.range = size;
		};

		void Destroy(
		) {
			vkDestroyBuffer(device->device, buffer, nullptr);
			vkFreeMemory(device->device, memory, nullptr);
		}
	};

	// ===================
	// GEOMETRIES
	// ===================
	struct GeometryBufferOffset {
		std::map<EVertexAttributeType, VkDeviceSize> vertexBufferOffsets;
	};

	struct GeometryBuffer {
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
