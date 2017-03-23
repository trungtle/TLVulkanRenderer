#include "VulkanUtil.h"
#include <algorithm>
#include <cassert>
#include <glfw3.h>
#include "VulkanSwapchain.h"
#include "Utilities.h"

namespace VulkanUtil {
	namespace Make {
		VkPresentInfoKHR
		MakePresentInfoKHR(
			const std::vector<VkSemaphore>& waitSemaphores,
			const std::vector<VkSwapchainKHR>& swapchain,
			const uint32_t* imageIndices
		) {
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = waitSemaphores.size();
			presentInfo.pWaitSemaphores = waitSemaphores.data();

			presentInfo.swapchainCount = swapchain.size();
			presentInfo.pSwapchains = swapchain.data();
			presentInfo.pImageIndices = imageIndices;

			return presentInfo;
		}

		VkDescriptorPoolSize
		MakeDescriptorPoolSize(
			VkDescriptorType type,
			uint32_t descriptorCount
		) {
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = type;
			poolSize.descriptorCount = descriptorCount;

			return poolSize;
		}

		VkDescriptorPoolCreateInfo
		MakeDescriptorPoolCreateInfo(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* poolSizes,
			uint32_t maxSets
		) {
			VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
			descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
			descriptorPoolCreateInfo.pPoolSizes = poolSizes;
			descriptorPoolCreateInfo.maxSets = maxSets;

			return descriptorPoolCreateInfo;
		}

		VkDescriptorSetLayoutBinding
		MakeDescriptorSetLayoutBinding(
			uint32_t binding,
			VkDescriptorType descriptorType,
			VkShaderStageFlags shaderFlags,
			uint32_t descriptorCount
		) {
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = binding;
			layoutBinding.descriptorType = descriptorType;
			layoutBinding.stageFlags = shaderFlags;
			layoutBinding.descriptorCount = descriptorCount;

			return layoutBinding;
		}

		VkDescriptorSetLayoutCreateInfo
		MakeDescriptorSetLayoutCreateInfo(
			VkDescriptorSetLayoutBinding* bindings,
			uint32_t bindingCount
		) {
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};

			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindingCount;
			layoutInfo.pBindings = bindings;

			return layoutInfo;
		}

		VkDescriptorSetAllocateInfo
		MakeDescriptorSetAllocateInfo(
			VkDescriptorPool descriptorPool,
			VkDescriptorSetLayout* descriptorSetLayout,
			uint32_t descriptorSetCount
		) {

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = descriptorSetLayout;

			return allocInfo;

		}

		VkDescriptorBufferInfo
		MakeDescriptorBufferInfo(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkDeviceSize range
		) {
			VkDescriptorBufferInfo descriptorBufferInfo = {};
			descriptorBufferInfo.buffer = buffer;
			descriptorBufferInfo.offset = offset;
			descriptorBufferInfo.range = range;

			return descriptorBufferInfo;
		}

		VkDescriptorImageInfo
		MakeDescriptorImageInfo(
			VkImageLayout layout,
			VulkanImage::Image& image
		) 
		{
			VkDescriptorImageInfo imageInfo;

			imageInfo.imageLayout = layout;
			imageInfo.imageView = image.imageView;
			imageInfo.sampler = image.sampler;
			
			image.descriptor = imageInfo;

			return imageInfo;
		}

		VkWriteDescriptorSet
		MakeWriteDescriptorSet(
			VkDescriptorType type,
			VkDescriptorSet dstSet,
			uint32_t dstBinding,
			uint32_t descriptorCount,
			VkDescriptorBufferInfo* bufferInfo,
			VkDescriptorImageInfo* imageInfo
		) {
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.dstBinding = dstBinding;
			writeDescriptorSet.dstArrayElement = 0; // descriptor set could be an array
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.descriptorCount = descriptorCount;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.pImageInfo = imageInfo;

			return writeDescriptorSet;
		}

		VkVertexInputBindingDescription
		MakeVertexInputBindingDescription(
			uint32_t binding,
			uint32_t stride,
			VkVertexInputRate rate
		) {
			VkVertexInputBindingDescription bindingDesc;
			bindingDesc.binding = binding;
			bindingDesc.stride = stride;
			bindingDesc.inputRate = rate;

			return bindingDesc;
		}

		VkVertexInputAttributeDescription
		MakeVertexInputAttributeDescription(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset
		) {
			VkVertexInputAttributeDescription attributeDesc;

			// Position attribute
			attributeDesc.binding = binding;
			attributeDesc.location = location;
			attributeDesc.format = format;
			attributeDesc.offset = offset;

			return attributeDesc;
		}

		VkPipelineVertexInputStateCreateInfo
		MakePipelineVertexInputStateCreateInfo(
			const std::vector<VkVertexInputBindingDescription>& bindingDesc,
			const std::vector<VkVertexInputAttributeDescription>& attribDesc
		) {
			VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = {};
			vertexInputStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			vertexInputStageCreateInfo.vertexBindingDescriptionCount = bindingDesc.size();
			vertexInputStageCreateInfo.pVertexBindingDescriptions = bindingDesc.data();
			vertexInputStageCreateInfo.vertexAttributeDescriptionCount = attribDesc.size();
			vertexInputStageCreateInfo.pVertexAttributeDescriptions = attribDesc.data();

			return vertexInputStageCreateInfo;
		}

		VkPipelineInputAssemblyStateCreateInfo
		MakePipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology
		) {
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
			inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCreateInfo.topology = topology;
			inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE; // If true, we can break up primitives like triangels and lines using a special index 0xFFFF

			return inputAssemblyStateCreateInfo;
		}

		VkViewport
		MakeFullscreenViewport(
			VkExtent2D extent
		) {
			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			return viewport;
		}

		VkPipelineViewportStateCreateInfo
		MakePipelineViewportStateCreateInfo(
			const std::vector<VkViewport>& viewports,
			const std::vector<VkRect2D>& scissors
		) {
			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = viewports.size();
			viewportStateCreateInfo.pViewports = viewports.data();
			viewportStateCreateInfo.scissorCount = scissors.size();
			viewportStateCreateInfo.pScissors = scissors.data();

			return viewportStateCreateInfo;
		}

		VkPipelineRasterizationStateCreateInfo
		MakePipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace
		) {
			VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
			rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			// If enabled, fragments beyond near and far planes are clamped instead of discarded
			rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			// If enabled, geometry won't pass through rasterization. This would be useful for transform feedbacks
			// where we don't need to go through the fragment shader
			rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationStateCreateInfo.polygonMode = polygonMode; // fill, line, or point
			rasterizationStateCreateInfo.lineWidth = 1.0f;
			rasterizationStateCreateInfo.cullMode = cullMode;
			rasterizationStateCreateInfo.frontFace = frontFace;
			rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
			rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
			rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
			rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

			return rasterizationStateCreateInfo;
		}

		VkPipelineMultisampleStateCreateInfo
		MakePipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits sampleCount
		) {
			VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
			multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleStateCreateInfo.rasterizationSamples = sampleCount;
			multisampleStateCreateInfo.minSampleShading = 1.0f;
			multisampleStateCreateInfo.pSampleMask = nullptr;
			multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
			multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

			return multisampleStateCreateInfo;
		}

		VkPipelineDepthStencilStateCreateInfo
		MakePipelineDepthStencilStateCreateInfo(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp compareOp
		) {
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
			depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
			depthStencilStateCreateInfo.depthCompareOp = compareOp; // 1.0f is farthest, 0.0f is closest
			depthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable; // Allowing for transparent objects
			depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE; // Allowing to keep fragment falling withn a  certain range
			depthStencilStateCreateInfo.minDepthBounds = 0.0f;
			depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
			depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
			depthStencilStateCreateInfo.front = {};
			depthStencilStateCreateInfo.back = {}; // For stencil test

			return depthStencilStateCreateInfo;
		}

		VkPipelineColorBlendStateCreateInfo
		MakePipelineColorBlendStateCreateInfo(
			const std::vector<VkPipelineColorBlendAttachmentState>& attachments
		) {
			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendStateCreateInfo.attachmentCount = attachments.size();
			colorBlendStateCreateInfo.pAttachments = attachments.data();

			return colorBlendStateCreateInfo;
		}

		VkPipelineDynamicStateCreateInfo
		MakePipelineDynamicStateCreateInfo(
			const VkDynamicState* pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags) 
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
			return pipelineDynamicStateCreateInfo;
		}

		VkPipelineLayoutCreateInfo
		MakePipelineLayoutCreateInfo(
			VkDescriptorSetLayout* descriptorSetLayouts,
			uint32_t setLayoutCount
		) {
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
			return pipelineLayoutCreateInfo;
		}

		VkComputePipelineCreateInfo
		MakeComputePipelineCreateInfo(VkPipelineLayout layout, VkPipelineCreateFlags flags) {
			VkComputePipelineCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			createInfo.layout = layout;
			createInfo.flags = flags;

			return createInfo;
		}

		VkShaderModule MakeShaderModule(const VkDevice& device, const std::string & filepath)
		{
			VkShaderModule module;

			std::vector<Byte> bytecode;
			LoadSPIR_V(filepath.c_str(), bytecode);

			VkResult result = VK_SUCCESS;

			VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
			shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCreateInfo.codeSize = bytecode.size();
			shaderModuleCreateInfo.pCode = (uint32_t*)bytecode.data();

			result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &module);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader module");
			}

			return module;
		}

		VkImageCreateInfo MakeImageCreateInfo(
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			VkImageType imageType,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage
		)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = imageType;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = depth;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.usage = usage;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // For multisampling
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // used only by one queue that supports transfer operations
			imageInfo.flags = 0; // We might look into this for flags that support sparse image (if we need to do voxel 3D texture for volumetric)

			return imageInfo;
		}

		VkRenderPassBeginInfo
		MakeRenderPassBeginInfo(
			const VkRenderPass& renderPass,
			const VkFramebuffer& framebuffer,
			const VkOffset2D& offset,
			const VkExtent2D& extent,
			const std::vector<VkClearValue>& clearValues
		) {
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = framebuffer;

			// The area where load and store takes place
			renderPassBeginInfo.renderArea.offset = offset;
			renderPassBeginInfo.renderArea.extent = extent;

			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();

			return renderPassBeginInfo;
		}

		VkPipelineShaderStageCreateInfo
		MakePipelineShaderStageCreateInfo(
			VkShaderStageFlagBits stage,
			const VkShaderModule& shaderModule
		) {
			VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo.stage = stage;
			shaderStageCreateInfo.module = shaderModule;
			// Specify entry point. It's possible to combine multiple shaders into a single shader module
			shaderStageCreateInfo.pName = "main";

			// This can be used to set values for shader constants. The compiler can perform optimization for these constants vs. if they're created as variables in the shaders.
			shaderStageCreateInfo.pSpecializationInfo = nullptr;

			return shaderStageCreateInfo;
		}

		VkGraphicsPipelineCreateInfo
		MakeGraphicsPipelineCreateInfo(
			const std::vector<VkPipelineShaderStageCreateInfo>& shaderCreateInfos,
			const VkPipelineVertexInputStateCreateInfo* vertexInputStage,
			const VkPipelineInputAssemblyStateCreateInfo* inputAssemblyState,
			const VkPipelineTessellationStateCreateInfo* tessellationState,
			const VkPipelineViewportStateCreateInfo* viewportState,
			const VkPipelineRasterizationStateCreateInfo* rasterizationState,
			const VkPipelineColorBlendStateCreateInfo* colorBlendState,
			const VkPipelineMultisampleStateCreateInfo* multisampleState,
			const VkPipelineDepthStencilStateCreateInfo* depthStencilState,
			const VkPipelineDynamicStateCreateInfo* dynamicState,
			const VkPipelineLayout pipelineLayout,
			const VkRenderPass renderPass,
			const uint32_t subpass,
			const VkPipeline basePipelineHandle,
			const int32_t basePipelineIndex
		) {
			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = shaderCreateInfos.size(); // Number of shader stages
			graphicsPipelineCreateInfo.pStages = shaderCreateInfos.data();
			graphicsPipelineCreateInfo.pVertexInputState = vertexInputStage;
			graphicsPipelineCreateInfo.pInputAssemblyState = inputAssemblyState;
			graphicsPipelineCreateInfo.pTessellationState = tessellationState;
			graphicsPipelineCreateInfo.pViewportState = viewportState;
			graphicsPipelineCreateInfo.pRasterizationState = rasterizationState;
			graphicsPipelineCreateInfo.pColorBlendState = colorBlendState;
			graphicsPipelineCreateInfo.pMultisampleState = multisampleState;
			graphicsPipelineCreateInfo.pDepthStencilState = depthStencilState;
			graphicsPipelineCreateInfo.pDynamicState = dynamicState;
			graphicsPipelineCreateInfo.layout = pipelineLayout;
			graphicsPipelineCreateInfo.renderPass = renderPass;
			graphicsPipelineCreateInfo.subpass = 0; // Index to the subpass we'll be using

			graphicsPipelineCreateInfo.basePipelineHandle = basePipelineHandle;
			graphicsPipelineCreateInfo.basePipelineIndex = basePipelineIndex;

			return graphicsPipelineCreateInfo;
		}

		void
		CreateDefaultImageSampler(
			const VkDevice& device,
			VkSampler* sampler
		) {
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.maxAnisotropy = 0;
			CheckVulkanResult(
				vkCreateSampler(device, &samplerCreateInfo, nullptr, sampler),
				"Failed to create texture sampler"
			);
		}

		VkSamplerCreateInfo 
		MakeSamplerCreateInfo(
			VkFilter magFilter, 
			VkFilter minFilter, 
			VkSamplerMipmapMode mipmapMode,
			VkSamplerAddressMode addressModeU, 
			VkSamplerAddressMode addressModeV, 
			VkSamplerAddressMode addressModeW, 
			float mipLodBias, 
			float maxAnisotropy, 
			float minLod, 
			float maxLod, 
			VkBorderColor borderColor
			) 
		{
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = magFilter;
			samplerCreateInfo.minFilter = minFilter;
			samplerCreateInfo.mipmapMode = mipmapMode;
			samplerCreateInfo.addressModeU = addressModeU;
			samplerCreateInfo.addressModeV = addressModeV;
			samplerCreateInfo.addressModeW = addressModeW;
			samplerCreateInfo.mipLodBias = mipLodBias;
			samplerCreateInfo.maxAnisotropy = maxAnisotropy;
			samplerCreateInfo.minLod = minLod;
			samplerCreateInfo.maxLod = maxLod;
			samplerCreateInfo.borderColor = borderColor;

			return samplerCreateInfo;
		}

		VkCommandPoolCreateInfo
		MakeCommandPoolCreateInfo(
			uint32_t queueFamilyIndex
		) {
			VkCommandPoolCreateInfo commandPoolCreateInfo = {};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

			// From Vulkan spec:
			// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT indicates that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
			// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT controls whether command buffers allocated from the pool can be individually reset.If this flag is set, individual command buffers allocated from the pool can be reset either explicitly, by calling vkResetCommandBuffer, or implicitly, by calling vkBeginCommandBuffer on an executable command buffer.If this flag is not set, then vkResetCommandBuffer and vkBeginCommandBuffer(on an executable command buffer) must not be called on the command buffers allocated from the pool, and they can only be reset in bulk by calling vkResetCommandPool.
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			return commandPoolCreateInfo;
		}

		VkCommandBufferAllocateInfo
		MakeCommandBufferAllocateInfo(
			VkCommandPool commandPool,
			VkCommandBufferLevel level,
			uint32_t bufferCount
		) {
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPool;
			// Primary means that can be submitted to a queue, but cannot be called from other command buffers
			allocInfo.level = level;
			allocInfo.commandBufferCount = bufferCount;

			return allocInfo;
		}

		VkCommandBufferBeginInfo
		MakeCommandBufferBeginInfo() {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

			return beginInfo;
		}

		VkSubmitInfo
		MakeSubmitInfo(
			const std::vector<VkSemaphore>& waitSemaphores,
			const std::vector<VkSemaphore>& signalSemaphores,
			const std::vector<VkPipelineStageFlags>& waitStageFlags,
			const VkCommandBuffer& commandBuffer
		) {
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			// Semaphore to wait on
			submitInfo.waitSemaphoreCount = waitSemaphores.size();
			submitInfo.pWaitSemaphores = waitSemaphores.data(); // The semaphore to wait on
			submitInfo.pWaitDstStageMask = waitStageFlags.data(); // At which stage to wait on

			// The command buffer to submit													
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Semaphore to signal
			submitInfo.signalSemaphoreCount = signalSemaphores.size();
			submitInfo.pSignalSemaphores = signalSemaphores.data();

			return submitInfo;
		}

		VkSubmitInfo
		MakeSubmitInfo(
			const VkCommandBuffer& commandBuffer
		) {
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			// The command buffer to submit													
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			return submitInfo;
		}

		VkFenceCreateInfo
		MakeFenceCreateInfo(
			VkFenceCreateFlags flags
		) {
			VkFenceCreateInfo createInfo = {};

			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createInfo.flags = flags;

			return createInfo;
		}
	}
}
