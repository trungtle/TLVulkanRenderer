/**
 * The basic of this is heavily referenced at:
 *  - Majority of this application was modified from Vulkan Tutorial(https://vulkan-tutorial.com/) by Alexander Overvoorde. [Github](https://github.com/Overv/VulkanTutorial). 
 *  - WSI Tutorial by Chris Hebert.
 *  - https://github.com/SaschaWillems/Vulkan by Sascha Willems.
 */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include "spdlog/spdlog.h"
#include "renderer/Renderer.h"
#include "VulkanDevice.h"
#include "VulkanUtil.h"
#include "VulkanBuffer.h"

using namespace VulkanUtil;
using namespace VulkanUtil::Make;

struct GraphicsUniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// ===================
// VULKAN RENDERER
// ===================

/**
 * \brief 
 */
class VulkanRenderer : public Renderer {
public:
	VulkanRenderer(
		GLFWwindow* window,
		Scene* scene,
		std::shared_ptr<std::map<string, string>> config
	);

	virtual ~VulkanRenderer() override;

	virtual void
	Prepare();
	
	virtual VkResult
	PrepareDescriptorPool();

	virtual VkResult
	PrepareDescriptorLayouts();

	virtual VkResult
	PrepareDescriptorSets();

	virtual VkResult
	PreparePipelines();

	virtual VkResult
	PrepareGraphicsCommandPool();

	virtual VkResult
	PrepareComputeCommandPool();

	virtual VkResult
	BuildCommandBuffers();

	virtual VkResult
	PrepareUniforms();

	virtual VkResult
	PrepareVertexBuffers();

	virtual void
	PrepareTextures();

	virtual void
	Update() override;

	virtual void
	Render() override;

protected:

	VkResult
	PrepareRenderPass();

	void
	GenerateWireframeBVHNodes();

	// ----------------
	// GRAPHICS PIPELINE
	// ----------------

	/**
	 * \brief The graphics pipeline are often fixed. Create a new pipeline if we need a different pipeline settings
	 * \return 
	 */
	virtual VkResult
	PrepareGraphicsPipeline();

	virtual VkResult
	PreparePostProcessingPipeline();

	// ----------------
	// SYCHRONIZATION
	// ----------------

	VkResult
	PrepareSemaphores();

	void
		PrepareFences();

	// ----------------
	// COMMAND BUFFER
	// ----------------
	void
		CreateCommandBuffers();

	void
		DestroyCommandBuffer();

	bool CheckCommandBuffers();

	/**
	* \brief Helper to determine the memory type to allocate from our graphics card
	* \param typeFilter
	* \param propertyFlags
	* \return
	*/

	VulkanDevice* m_vulkanDevice;

	/**
	* \brief Handles to the Vulkan present queue. This may or may not be the same as the graphics queue
	* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-queues
	*/
	VkQueue m_presentQueue;

	struct {

		/**
		* \brief Descriptor set layout to decribe our resource binding (ex. UBO)
		*/
		VkDescriptorSetLayout descriptorSetLayout;

		/**
		* \brief Descriptor pool for our resources
		*/
		VkDescriptorPool descriptorPool;

		/**
		* \brief Descriptor set for our resources
		*/
		VkDescriptorSet descriptorSet;

		/**
		* \brief This describes the uniforms inside shaders
		*/
		VkPipelineLayout pipelineLayout;

		/**
		* \brief Holds the renderpass object. This also represents the framebuffer attachments
		*/
		VkRenderPass renderPass;

		std::vector<VulkanBuffer::VertexBuffer> geometryBuffers;

		/**
		* \brief Uniform buffers
		*/
		VulkanBuffer::StorageBuffer uniformStaging;
		VulkanBuffer::StorageBuffer uniformStorage;

		/**
		* \brief Graphics pipeline
		*/
		VkPipeline m_pipeline;

		/**
		* \brief Command pool
		*/
		VkCommandPool commandPool;

		/**
		* \brief Command buffers to record our commands
		*/
		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkFence> fences;

		/**
		* \brief Handles to the Vulkan graphics queue. This may or may not be the same as the present queue
		* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-queues
		*/
		VkQueue queue;


	} m_graphics;

	struct Wireframe
	{
		// -- Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		// -- Descriptor
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;

		// count
		uint32_t indexCount;

		VulkanBuffer::StorageBuffer BVHVertices;
		VulkanBuffer::StorageBuffer BVHIndices;
		VulkanBuffer::StorageBuffer uniform;

	} m_wireframe;

	struct Compute
	{
		VkCommandPool commandPool;
		VkQueue queue;
	} m_compute;

	struct Quad
	{
		std::vector<uint16_t> indices;
		std::vector<vec2> positions;
		std::vector<vec2> uvs;
	} m_post_quad;


	/**
	 * \brief Semaphores to signal when to acquire and present swapchain images
	 */
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;

	/**
	 * \brief Logger
	 */
	std::shared_ptr<spdlog::logger> m_logger;
};
