#include "VulkanBuffer.h"
#include "VulkanDevice.h"

void VulkanBuffer::StorageBuffer::Create(const VulkanDevice* device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags memoryProperties) {
	m_device = device;
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
}

void VulkanBuffer::StorageBuffer::Destroy() const {
	vkDestroyBuffer(m_device->device, buffer, nullptr);
	vkFreeMemory(m_device->device, memory, nullptr);
}
