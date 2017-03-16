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
	Prepare() final;

	VkResult
	PreparePipelines() final;

	VkResult
	PrepareVertexBuffers() final;

	// --- Descriptor

	VkResult
	PrepareDescriptorPool() final;

	VkResult
	PrepareDescriptorLayouts() final;

	VkResult
	PrepareDescriptorSets() final;

	// --- Command buffers

	VkResult
	BuildCommandBuffers() final;

	// -----------
	// COMPUTE PIPELINE (for raytracing)
	// -----------

	void
	PrepareComputeRaytrace();

	void
	PrepareComputeRaytraceDescriptorSets();

	void
	PrepareComputeRaytraceCommandPool();

	void
	PrepareComputeRaytraceStorageBuffer();

	void
	PrepareComputeRaytraceUniformBuffer();

	VkResult
	PrepareComputeRaytraceTextureResources();

	VkResult
	PrepareComputeRaytracePipeline();

	VkResult
	BuildComputeCommandBuffers();

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
