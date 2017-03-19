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
#include "VulkanSwapchain.h"
#include "VulkanUtil.h"
#include "Typedef.h"
#include "VulkanImage.h"
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
	PrepareCommandPool();

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
		VkDescriptorSet descriptorSets;

		/**
		* \brief This describes the uniforms inside shaders
		*/
		VkPipelineLayout pipelineLayout;

		/**
		* \brief Holds the renderpass object. This also represents the framebuffer attachments
		*/
		VkRenderPass renderPass;

		std::vector<VulkanBuffer::GeometryBuffer> geometryBuffers;

		/**
		* \brief Uniform buffers
		*/
		VkBuffer m_uniformStagingBuffer;
		VkBuffer m_uniformBuffer;
		VkDeviceMemory m_uniformStagingBufferMemory;
		VkDeviceMemory m_uniformBufferMemory;

		/**
		* \brief Graphics pipeline
		*/
		VkPipeline m_graphicsPipeline;

		/**
		* \brief Command pool
		*/
		VkCommandPool commandPool;

		/**
		* \brief Command buffers to record our commands
		*/
		std::vector<VkCommandBuffer> commandBuffers;

		/**
		* \brief Handles to the Vulkan graphics queue. This may or may not be the same as the present queue
		* \ref https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/xhtml/vkspec.html#devsandqueues-queues
		*/
		VkQueue queue;


	} m_graphics;

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
