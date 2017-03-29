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

void VulkanBuffer::VertexBuffer::Create(
	const VulkanDevice* vulkanDevice,
	VertexData* vertexData
)
{
	// At the mimum we always have indices and position
	VkDeviceSize offset = 0;

	// -- Indices
	std::vector<Byte>& indexData = vertexData->bytes.at(INDEX);
	VkDeviceSize indexBufferSize = sizeof(indexData[0]) * indexData.size();
	VkDeviceSize indexBufferOffset = offset;
	indexCount = indexData.size();
	offset += indexBufferSize;

	// -- Positions
	std::vector<Byte>& positionData = vertexData->bytes.at(POSITION);
	VkDeviceSize positionBufferSize = sizeof(positionData[0]) * positionData.size();
	VkDeviceSize positionBufferOffset = offset;
	offset += positionBufferSize;

	VkDeviceSize bufferSize = indexBufferSize + positionBufferSize;

	// -- Normals
	std::vector<Byte> normalData;
	VkDeviceSize normalBufferSize = 0, normalBufferOffset = 0;
	if (vertexData->bytes.find(NORMAL) != vertexData->bytes.end())
	{
		normalData = vertexData->bytes.at(NORMAL);
		normalBufferSize = sizeof(normalData[0]) * normalData.size();
		normalBufferOffset = offset;
		offset += normalBufferSize;
		bufferSize += normalBufferSize;
	}

	// -- UVs
	std::vector<Byte> uvData;
	VkDeviceSize uvBufferSize = 0;
	VkDeviceSize uvBufferOffset = 0;
	if (vertexData->bytes.find(TEXCOORD) != vertexData->bytes.end())
	{
		uvData = vertexData->bytes.at(TEXCOORD);
		uvBufferSize = sizeof(uvData[0]) * uvData.size();
		uvBufferOffset = offset;
		offset += uvBufferSize;
		bufferSize += uvBufferSize;
	}
	std::vector<Byte> materialIDData;
	VkDeviceSize materialIDBufferSize = 0, materialIDBufferOffset = 0;
	if (vertexData->bytes.find(MATERIALID) != vertexData->bytes.end())
	{
		materialIDData = vertexData->bytes.at(MATERIALID);
		materialIDBufferSize = sizeof(materialIDData[0]) * materialIDData.size();
		materialIDBufferOffset = offset;
		offset += materialIDBufferSize;
		bufferSize += materialIDBufferSize;
	}

	this->offsets.insert(std::make_pair(INDEX, indexBufferOffset));
	this->offsets.insert(std::make_pair(POSITION, positionBufferOffset));
	this->offsets.insert(std::make_pair(NORMAL, normalBufferOffset));
	this->offsets.insert(std::make_pair(TEXCOORD, uvBufferOffset));
	this->offsets.insert(std::make_pair(MATERIALID, materialIDBufferOffset));

	// Stage buffer memory on host
	// We want staging so that we can map the vertex data on the host but
	// then transfer it to the device local memory for faster performance
	// This is the recommended way to allocate buffer memory,
	VulkanBuffer::StorageBuffer staging;

	staging.Create(
		vulkanDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// Filling the stage buffer with data
	void* data;
	vkMapMemory(vulkanDevice->device, staging.memory, 0, bufferSize, 0, &data);
	memcpy(data, indexData.data(), static_cast<size_t>(indexBufferSize));
	memcpy((Byte*)data + positionBufferOffset, positionData.data(), static_cast<size_t>(positionBufferSize));
	if (vertexData->bytes.find(NORMAL) != vertexData->bytes.end())
	{
		memcpy((Byte*)data + normalBufferOffset, normalData.data(), static_cast<size_t>(normalBufferSize));
	}
	if (vertexData->bytes.find(TEXCOORD) != vertexData->bytes.end())
	{
		memcpy((Byte*)data + uvBufferOffset, uvData.data(), static_cast<size_t>(uvBufferSize));
	}
	if (vertexData->bytes.find(MATERIALID) != vertexData->bytes.end())
	{
		memcpy((Byte*)data + materialIDBufferOffset, materialIDData.data(), static_cast<size_t>(materialIDBufferSize));
	}
	vkUnmapMemory(vulkanDevice->device, staging.memory);

	// -----------------------------------------

	this->storageBuffer.Create(
		vulkanDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// Copy over to vertex buffer in device local memory
	vulkanDevice->CopyBuffer(
		this->storageBuffer,
		staging,
		bufferSize
	);

	// Cleanup staging buffer memory
	staging.Destroy();

}

void 
VulkanBuffer::VertexBuffer::CreateWireframe(
	const VulkanDevice* device, 
	const std::vector<uint16_t>& indices, 
	const std::vector<SWireframeVertexLayout>& vertices
	) 
{
	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
	VkDeviceSize indexBufferOffset = 0;
	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
	VkDeviceSize vertexBufferOffset = indexBufferSize;

	VkDeviceSize bufferSize = indexBufferSize + vertexBufferSize;
	this->offsets.insert(std::make_pair(INDEX, indexBufferOffset));
	this->offsets.insert(std::make_pair(WIREFRAME, vertexBufferOffset));

	// Stage buffer memory on host
	// We want staging so that we can map the vertex data on the host but
	// then transfer it to the device local memory for faster performance
	// This is the recommended way to allocate buffer memory,
	VulkanBuffer::StorageBuffer staging;
	staging.Create(
		device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// Filling the stage buffer with data
	void* data;
	vkMapMemory(device->device, staging.memory, 0, bufferSize, 0, &data);
	memcpy((Byte*)data, (Byte*)indices.data(), static_cast<size_t>(indexBufferSize));
	memcpy((Byte*)data + vertexBufferOffset, (Byte*)vertices.data(), static_cast<size_t>(vertexBufferSize));
	vkUnmapMemory(device->device, staging.memory);

	// -----------------------------------------

	this->storageBuffer.Create(
		device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// Copy over to vertex buffer in device local memory
	device->CopyBuffer(
		this->storageBuffer,
		staging,
		bufferSize
	);

	// Cleanup staging buffer memory
	staging.Destroy();

	indexCount = indices.size();
}
