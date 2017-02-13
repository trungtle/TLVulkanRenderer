#pragma once
#include "VulkanRenderer.h"
#include "VulkanBuffer.h"

class VulkanGPURaytracer : public VulkanRenderer {

public:
	VulkanGPURaytracer(
		GLFWwindow* window,
		Scene* scene,
		std::shared_ptr<std::map<string, string>> config
	);

	virtual void
	Update() final;

	virtual void
	Render() final;

	virtual ~VulkanGPURaytracer() final;

protected:

	// -----------
	// GRAPHICS PIPELINE
	// -----------
	void
	PrepareGraphics() final;

	VkResult
	PrepareGraphicsPipeline() final;

	VkResult
	PrepareGraphicsVertexBuffer() final;

	// --- Descriptor

	VkResult
	PrepareGraphicsDescriptorPool() final;

	VkResult
	PrepareGraphicsDescriptorSetLayout() final;

	VkResult
	PrepareGraphicsDescriptorSets() final;

	// --- Command buffers

	VkResult
	PrepareGraphicsCommandBuffers() final;

	// -----------
	// COMPUTE PIPELINE (for raytracing)
	// -----------

	void
	PrepareCompute() final;

	void
	PrepareComputeDescriptors();

	void
	PrepareComputeCommandPool();

	void
	PrepareComputeStorageBuffer();

	void
	PrepareComputeUniformBuffer();

	VkResult
	PrepareRayTraceTextureResources();

	VkResult
	PrepareComputePipeline();

	VkResult
	PrepareComputeCommandBuffers();

	struct Quad {
		std::vector<uint16_t> indices;
		std::vector<vec2> positions;
		std::vector<vec2> uvs;
	} m_quad;


	struct Compute {
		// -- Compute compatible queue
		VkQueue queue;
		VkFence fence;

		// -- Descriptor
		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSets;

		// -- Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// -- Commands
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		struct {
			// -- Uniform buffer
			VulkanBuffer::StorageBuffer uniform;
			VulkanBuffer::StorageBuffer stagingUniform;
			VulkanBuffer::StorageBuffer materials;

			// -- Shapes buffers
			VulkanBuffer::StorageBuffer indices;
			VulkanBuffer::StorageBuffer verticePositions;
			VulkanBuffer::StorageBuffer verticeNormals;

		} buffers;

		// -- Output storage image
		VulkanImage::Image storageRaytraceImage;

		// -- Uniforms
		struct UBOCompute { // Compute shader uniform block object
			glm::vec4 position = glm::vec4(0.0, 2.5f, 15.0f, 1.0f);
			glm::vec4 right = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);;
			glm::vec4 lookat = glm::vec4(0.0, 2.5f, 0.0f, 0.0f);
			glm::vec4 forward;
			glm::vec4 up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec2 pixelLength;
			float fov = 40.0f;
			float aspectRatio = 45.0f;
		} ubo;

	} m_compute;
};
