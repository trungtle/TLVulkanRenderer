#include <assert.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "VulkanRenderer.h"
#include "Utilities.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"

VulkanRenderer::VulkanRenderer(
	GLFWwindow* window,
	Scene* scene
)
	:
	Renderer(window, scene) {
	// -- Initialize logger

	// Combine console and file logger
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
	// Create a 5MB rotating logger
	sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_st>("VulkanRenderer", "log", 1024 * 1024 * 5, 3));
	m_logger = std::make_shared<spdlog::logger>("Logger", begin(sinks), end(sinks));
	m_logger->set_pattern("<%H:%M:%S>[%I] %v");

	// -- Initialize Vulkan

	m_vulkanDevice = new VulkanDevice(m_window, "Vulkan renderer", m_logger);
	m_height = m_vulkanDevice->m_swapchain.extent.height;
	m_width = m_vulkanDevice->m_swapchain.extent.width;

	// Grabs the first queue in the graphics queue family since we only need one graphics queue anyway
	vkGetDeviceQueue(m_vulkanDevice->device, m_vulkanDevice->queueFamilyIndices.graphicsFamily, 0, &m_graphics.queue);

	// Grabs the first queue in the present queue family since we only need one present queue anyway
	vkGetDeviceQueue(m_vulkanDevice->device, m_vulkanDevice->queueFamilyIndices.presentFamily, 0, &m_presentQueue);

	VkResult result;
	result = PrepareGraphicsCommandPool();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created command pool");

	result = m_vulkanDevice->PrepareDepthResources(m_graphics.queue, m_graphics.commandPool);
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created depth image");

	result = PrepareRenderPass();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created renderpass");

	result = m_vulkanDevice->PrepareFramebuffers(m_graphics.renderPass);
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created framebuffers");

	result = PrepareSemaphores();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created semaphores");

	// -- Prepare compute work
	PrepareCompute();

	// -- Prepare graphics specific work
	PrepareGraphics();

}

VulkanRenderer::~VulkanRenderer() {

	// Flush device to make sure all resources can be freed 
	vkDeviceWaitIdle(m_vulkanDevice->device);

	// Free memory in the opposite order of creation

	vkDestroySemaphore(m_vulkanDevice->device, m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_vulkanDevice->device, m_renderFinishedSemaphore, nullptr);

	vkFreeCommandBuffers(m_vulkanDevice->device, m_graphics.commandPool, m_graphics.commandBuffers.size(), m_graphics.commandBuffers.data());

	vkDestroyDescriptorPool(m_vulkanDevice->device, m_graphics.descriptorPool, nullptr);

	for (VulkanBuffer::GeometryBuffer& geomBuffer : m_graphics.geometryBuffers) {
		vkFreeMemory(m_vulkanDevice->device, geomBuffer.vertexBufferMemory, nullptr);
		vkDestroyBuffer(m_vulkanDevice->device, geomBuffer.vertexBuffer, nullptr);
	}

	vkFreeMemory(m_vulkanDevice->device, m_graphics.m_uniformStagingBufferMemory, nullptr);
	vkDestroyBuffer(m_vulkanDevice->device, m_graphics.m_uniformStagingBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_graphics.m_uniformBufferMemory, nullptr);
	vkDestroyBuffer(m_vulkanDevice->device, m_graphics.m_uniformBuffer, nullptr);

	vkDestroyCommandPool(m_vulkanDevice->device, m_graphics.commandPool, nullptr);
	for (auto& frameBuffer : m_vulkanDevice->m_swapchain.framebuffers) {
		vkDestroyFramebuffer(m_vulkanDevice->device, frameBuffer, nullptr);
	}

	vkDestroyRenderPass(m_vulkanDevice->device, m_graphics.renderPass, nullptr);

	vkDestroyDescriptorSetLayout(m_vulkanDevice->device, m_graphics.descriptorSetLayout, nullptr);


	vkDestroyPipelineLayout(m_vulkanDevice->device, m_graphics.pipelineLayout, nullptr);
	for (auto& imageView : m_vulkanDevice->m_swapchain.imageViews) {
		vkDestroyImageView(m_vulkanDevice->device, imageView, nullptr);
	}
	vkDestroyPipeline(m_vulkanDevice->device, m_graphics.m_graphicsPipeline, nullptr);

	delete m_vulkanDevice;
}


VkResult
VulkanRenderer::PrepareRenderPass() {
	// ----- Specify color attachment ------

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_vulkanDevice->m_swapchain.imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// What to do with the color attachment before loading rendered content
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear to black
	// What to do after rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Read back rendered image
	// Stencil ops
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Pixel layout of VkImage objects in memory
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // To be presented in swapchain

	// Create subpasses. A phase of rendering within a render pass, that reads and writes a subset of the attachments
	// Subpass can depend on previous rendered framebuffers, so it could be used for post-processing.
	// It uses the attachment reference of the color attachment specified above.
	VkAttachmentReference colorAttachmentRef = {};
	// Index to which attachment in the attachment descriptions array. This is equivalent to the attachment at layout(location = 0)
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// ------ Depth attachment -------

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VulkanImage::FindDepthFormat(m_vulkanDevice->physicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Store if we're going to do deferred rendering
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// ------ Subpass CreateBuffer-------

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Make this subpass binds to graphics point
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Subpass dependency
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	// Wait until the swapchain finishes before reading it
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// Read and write to color attachment
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// ------ Renderpass creation -------

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VkResult result = vkCreateRenderPass(m_vulkanDevice->device, &renderPassCreateInfo, nullptr, &m_graphics.renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
		return result;
	}

	return result;
}

VkResult
VulkanRenderer::PrepareGraphicsDescriptorSetLayout() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: Fragment shader image sampler
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT
		),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
			MakeDescriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size()
			);

	VkResult result = vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_graphics.descriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
		return result;
	}

	return result;
}

VkResult
VulkanRenderer::PrepareGraphicsPipeline() {

	VkResult result = VK_SUCCESS;

	// Load SPIR-V bytecode
	// The SPIR_V files can be compiled by running glsllangValidator.exe from the VulkanSDK or
	// by invoking the custom script shaders/compileShaders.bat
	VkShaderModule vertShader;
	PrepareShaderModule("shaders/vert.spv", vertShader);
	VkShaderModule fragShader;
	PrepareShaderModule("shaders/frag.spv", fragShader);

	// -- VERTEX INPUT STAGE
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineVertexInputStateCreateInfo
	// Input binding description
	VertexAttributeInfo positionAttrib = m_scene->meshesData[0]->vertexAttributes.at(POSITION);
	VertexAttributeInfo normalAttrib = m_scene->meshesData[0]->vertexAttributes.at(NORMAL);
	std::vector<VkVertexInputBindingDescription> bindingDesc = {
		MakeVertexInputBindingDescription(
			0, // binding
			positionAttrib.byteStride,
			VK_VERTEX_INPUT_RATE_VERTEX
		),
		MakeVertexInputBindingDescription(
			1, // binding
			normalAttrib.byteStride,
			VK_VERTEX_INPUT_RATE_VERTEX
		)
	};


	// Attribute description (position, normal, texcoord etc.)
	std::vector<VkVertexInputAttributeDescription> attribDesc = {
		MakeVertexInputAttributeDescription(
			0, // binding
			0, // location
			VK_FORMAT_R32G32B32_SFLOAT,
			0 // offset
		),
		MakeVertexInputAttributeDescription(
			1, // binding
			1, // location
			VK_FORMAT_R32G32B32_SFLOAT,
			0 // offset
		)
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = MakePipelineVertexInputStateCreateInfo(
		bindingDesc,
		attribDesc
	);


	// -- INPUT ASSEMBLY
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineInputAssemblyStateCreateInfo
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
			MakePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// -- TESSELATION
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineTessellationStateCreateInfo


	// -- VIEWPORTS & SCISSORS
	// Viewport typically just covers the entire swapchain extent
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineViewportStateCreateInfo
	std::vector<VkViewport> viewports = {
		MakeFullscreenViewport(m_vulkanDevice->m_swapchain.extent)
	};

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = m_vulkanDevice->m_swapchain.extent;

	std::vector<VkRect2D> scissors = {
		scissor
	};

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = MakePipelineViewportStateCreateInfo(viewports, scissors);

	// -- RASTERIZATION
	// This stage converts primitives into fragments. It also performs depth/stencil testing, face culling, scissor test.
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineRasterizationStateCreateInfo 
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = MakePipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// -- MULTISAMPLING
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineMultisampleStateCreateInfo

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
			MakePipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	// -- DEPTH AND STENCIL TEST
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineDepthStencilStateCreateInfo
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = MakePipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	// -- COLOR BLENDING
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineColorBlendStateCreateInfo
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Do minimal work for now, so no blending. This will be useful for later on when we want to do pixel blending (alpha blending).
	// There are a lot of interesting blending we can do here.
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {
		colorBlendAttachmentState
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = MakePipelineColorBlendStateCreateInfo(colorBlendAttachments);

	// -- DYNAMIC STATE
	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState =
		MakePipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			static_cast<uint32_t>(dynamicStateEnables.size()),
			0);

	// 9. Create pipeline layout to hold uniforms. This can be modified dynamically. 
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_graphics.descriptorSetLayout);
	pipelineLayoutCreateInfo.pSetLayouts = &m_graphics.descriptorSetLayout;
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_graphics.pipelineLayout),
		"Failed to create pipeline layout."
	);

	// -- Setup the programmable stages for the pipeline. This links the shader modules with their corresponding stages.
	// \ref https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineShaderStageCreateInfo

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos = {
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	};

	// Finally, create our graphics pipeline here!

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
			MakeGraphicsPipelineCreateInfo(
				shaderCreateInfos,
				&vertexInputStageCreateInfo,
				&inputAssemblyStateCreateInfo,
				nullptr,
				&viewportStateCreateInfo,
				&rasterizationStateCreateInfo,
				&colorBlendStateCreateInfo,
				&multisampleStateCreateInfo,
				&depthStencilStateCreateInfo,
				nullptr,
				m_graphics.pipelineLayout,
				m_graphics.renderPass,
				0, // Subpass

				// Since pipelins are expensive to create, potentially we could reuse a common parent pipeline using the base pipeline handle.									
				// We just have one here so we don't need to specify these values.
				VK_NULL_HANDLE,
				-1
			);

	// We can also cache the pipeline object and store it in a file for resuse
	CheckVulkanResult(
		vkCreateGraphicsPipelines(
			m_vulkanDevice->device,
			VK_NULL_HANDLE, // Pipeline caches here
			1, // Pipeline count
			&graphicsPipelineCreateInfo,
			nullptr,
			&m_graphics.m_graphicsPipeline // Pipelines
		),
		"Failed to create graphics pipeline"
	);

	// We don't need the shader modules after the graphics pipeline creation. Destroy them now.
	vkDestroyShaderModule(m_vulkanDevice->device, vertShader, nullptr);
	vkDestroyShaderModule(m_vulkanDevice->device, fragShader, nullptr);

	return result;
}

VkResult
VulkanRenderer::PrepareGraphicsCommandPool() {
	VkResult result = VK_SUCCESS;

	// Command pool for the graphics queue
	VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo = MakeCommandPoolCreateInfo(m_vulkanDevice->queueFamilyIndices.graphicsFamily);

	result = vkCreateCommandPool(m_vulkanDevice->device, &graphicsCommandPoolCreateInfo, nullptr, &m_graphics.commandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool.");
		return result;
	}

	return result;
}

VkResult
VulkanRenderer::PrepareGraphicsVertexBuffer() {
	m_graphics.geometryBuffers.clear();

	for (MeshData* geomData : m_scene->meshesData) {
		VulkanBuffer::GeometryBuffer geomBuffer;

		// ----------- Vertex attributes --------------

		std::vector<Byte>& indexData = geomData->vertexData.at(INDEX);
		VkDeviceSize indexBufferSize = sizeof(indexData[0]) * indexData.size();
		VkDeviceSize indexBufferOffset = 0;
		std::vector<Byte>& positionData = geomData->vertexData.at(POSITION);
		VkDeviceSize positionBufferSize = sizeof(positionData[0]) * positionData.size();
		VkDeviceSize positionBufferOffset = indexBufferSize;
		std::vector<Byte>& normalData = geomData->vertexData.at(NORMAL);
		VkDeviceSize normalBufferSize = sizeof(normalData[0]) * normalData.size();
		VkDeviceSize normalBufferOffset = positionBufferOffset + positionBufferSize;

		VkDeviceSize bufferSize = indexBufferSize + positionBufferSize + normalBufferSize;
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(INDEX, indexBufferOffset));
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(POSITION, positionBufferOffset));
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(NORMAL, normalBufferOffset));

		// Stage buffer memory on host
		// We want staging so that we can map the vertex data on the host but
		// then transfer it to the device local memory for faster performance
		// This is the recommended way to allocate buffer memory,
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		m_vulkanDevice->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBuffer
		);

		// Allocate memory for the buffer
		m_vulkanDevice->CreateMemory(
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		// Bind buffer with memory
		VkDeviceSize memoryOffset = 0;
		vkBindBufferMemory(m_vulkanDevice->device, stagingBuffer, stagingBufferMemory, memoryOffset);

		// Filling the stage buffer with data
		void* data;
		vkMapMemory(m_vulkanDevice->device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indexData.data(), static_cast<size_t>(indexBufferSize));
		memcpy((Byte*)data + positionBufferOffset, positionData.data(), static_cast<size_t>(positionBufferSize));
		memcpy((Byte*)data + normalBufferOffset, normalData.data(), static_cast<size_t>(normalBufferSize));
		vkUnmapMemory(m_vulkanDevice->device, stagingBufferMemory);

		// -----------------------------------------

		m_vulkanDevice->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			geomBuffer.vertexBuffer
		);

		// Allocate memory for the buffer
		m_vulkanDevice->CreateMemory(
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			geomBuffer.vertexBuffer,
			geomBuffer.vertexBufferMemory
		);

		// Bind buffer with memory
		vkBindBufferMemory(m_vulkanDevice->device, geomBuffer.vertexBuffer, geomBuffer.vertexBufferMemory, memoryOffset);

		// Copy over to vertex buffer in device local memory
		m_vulkanDevice->CopyBuffer(
			m_graphics.queue,
			m_graphics.commandPool,
			geomBuffer.vertexBuffer,
			stagingBuffer,
			bufferSize
		);

		// Cleanup staging buffer memory
		vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer, nullptr);
		vkFreeMemory(m_vulkanDevice->device, stagingBufferMemory, nullptr);

		m_graphics.geometryBuffers.push_back(geomBuffer);
	}
	return VK_SUCCESS;
}


VkResult
VulkanRenderer::PrepareGraphicsUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(GraphicsUniformBufferObject);
	VkDeviceSize memoryOffset = 0;
	m_vulkanDevice->CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		m_graphics.m_uniformStagingBuffer
	);

	// Allocate memory for the buffer
	m_vulkanDevice->CreateMemory(
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_graphics.m_uniformStagingBuffer,
		m_graphics.m_uniformStagingBufferMemory
	);

	// Bind buffer with memory
	vkBindBufferMemory(m_vulkanDevice->device, m_graphics.m_uniformStagingBuffer, m_graphics.m_uniformStagingBufferMemory, memoryOffset);

	m_vulkanDevice->CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		m_graphics.m_uniformBuffer
	);

	// Allocate memory for the buffer
	m_vulkanDevice->CreateMemory(
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_graphics.m_uniformBuffer,
		m_graphics.m_uniformBufferMemory
	);

	// Bind buffer with memory
	vkBindBufferMemory(m_vulkanDevice->device, m_graphics.m_uniformBuffer, m_graphics.m_uniformBufferMemory, memoryOffset);

	return VK_SUCCESS;
}

VkResult
VulkanRenderer::PrepareGraphicsDescriptorPool() {
	VkDescriptorPoolSize poolSize = MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(1, &poolSize, 1);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_graphics.descriptorPool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

VkResult
VulkanRenderer::PrepareGraphicsDescriptorSets() {
	VkDescriptorSetAllocateInfo allocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_graphics.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &allocInfo, &m_graphics.descriptorSets),
		"Failed to allocate descriptor set"
	);

	VkDescriptorBufferInfo bufferInfo = MakeDescriptorBufferInfo(m_graphics.m_uniformBuffer, 0, sizeof(GraphicsUniformBufferObject));

	// Update descriptor set info
	VkWriteDescriptorSet descriptorWrite = MakeWriteDescriptorSet(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		m_graphics.descriptorSets,
		0,
		1,
		&bufferInfo,
		nullptr
	);

	vkUpdateDescriptorSets(m_vulkanDevice->device, 1, &descriptorWrite, 0, nullptr);

	return VK_SUCCESS;
}

VkResult
VulkanRenderer::PrepareGraphicsCommandBuffers() {
	m_graphics.commandBuffers.resize(m_vulkanDevice->m_swapchain.framebuffers.size());
	// Primary means that can be submitted to a queue, but cannot be called from other command buffers
	VkCommandBufferAllocateInfo allocInfo = MakeCommandBufferAllocateInfo(m_graphics.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vulkanDevice->m_swapchain.framebuffers.size());

	CheckVulkanResult(
		vkAllocateCommandBuffers(m_vulkanDevice->device, &allocInfo, m_graphics.commandBuffers.data()),
		"Failed to create command buffers."
	);

	for (int i = 0; i < m_graphics.commandBuffers.size(); ++i) {
		// Begin command recording
		VkCommandBufferBeginInfo beginInfo = MakeCommandBufferBeginInfo();

		vkBeginCommandBuffer(m_graphics.commandBuffers[i], &beginInfo);

		// Begin renderpass
		std::vector<VkClearValue> clearValues(2);
		clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};
		VkRenderPassBeginInfo renderPassBeginInfo = MakeRenderPassBeginInfo(
			m_graphics.renderPass,
			m_vulkanDevice->m_swapchain.framebuffers[i],
			{0, 0},
			m_vulkanDevice->m_swapchain.extent,
			clearValues
		);

		// Record begin renderpass
		vkCmdBeginRenderPass(m_graphics.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Record binding the graphics pipeline
		vkCmdBindPipeline(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.m_graphicsPipeline);

		for (int b = 0; b < m_graphics.geometryBuffers.size(); ++b) {
			VulkanBuffer::GeometryBuffer& geomBuffer = m_graphics.geometryBuffers[b];

			// Bind vertex buffer
			VkBuffer vertexBuffers[] = {geomBuffer.vertexBuffer, geomBuffer.vertexBuffer};
			VkDeviceSize offsets[] = {geomBuffer.bufferLayout.vertexBufferOffsets.at(POSITION), geomBuffer.bufferLayout.vertexBufferOffsets.at(NORMAL)};
			vkCmdBindVertexBuffers(m_graphics.commandBuffers[i], 0, 2, vertexBuffers, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(m_graphics.commandBuffers[i], geomBuffer.vertexBuffer, geomBuffer.bufferLayout.vertexBufferOffsets.at(INDEX), VK_INDEX_TYPE_UINT16);

			// Bind uniform buffer
			vkCmdBindDescriptorSets(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.pipelineLayout, 0, 1, &m_graphics.descriptorSets, 0, nullptr);

			// Record draw command for the triangle!
			vkCmdDrawIndexed(m_graphics.commandBuffers[i], m_scene->meshesData[b]->vertexAttributes.at(INDEX).count, 1, 0, 0, 0);
		}

		// Record end renderpass
		vkCmdEndRenderPass(m_graphics.commandBuffers[i]);

		// End command recording
		CheckVulkanResult(
			vkEndCommandBuffer(m_graphics.commandBuffers[i]),
			"Failed to record command buffers"
		);
	}

	return VK_SUCCESS;
}

VkResult
VulkanRenderer::PrepareSemaphores() {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(m_vulkanDevice->device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphore);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create imageAvailable semaphore");
		return result;
	}

	result = vkCreateSemaphore(m_vulkanDevice->device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphore);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create renderFinished semaphore");
		return result;
	}

	return result;
}

VkResult
VulkanRenderer::PrepareShaderModule(
	const std::string& filepath,
	VkShaderModule& shaderModule
) const {
	std::vector<Byte> bytecode;
	LoadSPIR_V(filepath.c_str(), bytecode);

	VkResult result = VK_SUCCESS;

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = bytecode.size();
	shaderModuleCreateInfo.pCode = (uint32_t*)bytecode.data();

	result = vkCreateShaderModule(m_vulkanDevice->device, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	return result;
}

void
VulkanRenderer::PrepareGraphics() {
	VkResult result;

	result = PrepareGraphicsVertexBuffer();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created vertex buffer");

	result = PrepareGraphicsUniformBuffer();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created graphics uniform buffer");

	result = PrepareGraphicsDescriptorPool();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created descriptor pool");

	result = PrepareGraphicsDescriptorSetLayout();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created descriptor set layout");

	result = PrepareGraphicsDescriptorSets();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created descriptor set");

	result = PrepareGraphicsPipeline();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created graphics pipeline");

	result = PrepareGraphicsCommandBuffers();
	assert(result == VK_SUCCESS);
	m_logger->info<std::string>("Created command buffers");

}

void
VulkanRenderer::Update() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float timeSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	GraphicsUniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), timeSeconds * glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 1.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), (float)m_vulkanDevice->m_swapchain.extent.width / (float)m_vulkanDevice->m_swapchain.extent.height, 0.001f, 10000.0f);

	// The Vulkan's Y coordinate is flipped from OpenGL (glm design), so we need to invert that
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(m_vulkanDevice->device, m_graphics.m_uniformStagingBufferMemory, 0, sizeof(GraphicsUniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(GraphicsUniformBufferObject));
	vkUnmapMemory(m_vulkanDevice->device, m_graphics.m_uniformStagingBufferMemory);

	m_vulkanDevice->CopyBuffer(
		m_graphics.queue,
		m_graphics.commandPool,
		m_graphics.m_uniformBuffer,
		m_graphics.m_uniformStagingBuffer,
		sizeof(GraphicsUniformBufferObject));
}

void
VulkanRenderer::Render() {
	// Acquire the swapchain
	uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_vulkanDevice->device,
		m_vulkanDevice->m_swapchain.swapchain,
		UINT64_MAX, // Timeout
		m_imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&imageIndex
	);

	// Submit command buffers
	std::vector<VkSemaphore> waitSemaphores = {m_imageAvailableSemaphore};
	std::vector<VkSemaphore> signalSemaphores = {m_renderFinishedSemaphore};
	std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo = MakeSubmitInfo(
		waitSemaphores,
		signalSemaphores,
		waitStages,
		m_graphics.commandBuffers[imageIndex]
	);

	// Submit to queue
	CheckVulkanResult(
		vkQueueSubmit(m_graphics.queue, 1, &submitInfo, VK_NULL_HANDLE),
		"Failed to submit queue"
	);

	// Present swapchain image! Use the signal semaphore for present swapchain to wait for the next one
	std::vector<VkSwapchainKHR> swapchains = {m_vulkanDevice->m_swapchain.swapchain};
	VkPresentInfoKHR presentInfo = MakePresentInfoKHR(
		signalSemaphores,
		swapchains,
		&imageIndex
	);

	vkQueuePresentKHR(m_graphics.queue, &presentInfo);
}
