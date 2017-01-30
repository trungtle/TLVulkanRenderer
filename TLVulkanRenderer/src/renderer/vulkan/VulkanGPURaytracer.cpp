#include "VulkanGPURaytracer.h"
#include "Utilities.h"
#include "scene/Camera.h"

VulkanGPURaytracer::VulkanGPURaytracer(
	GLFWwindow* window,
	Scene* scene): VulkanRenderer(window, scene) {

	PrepareCompute();
	PrepareGraphics();
}

void
VulkanGPURaytracer::Update() {
	// Update camera ubo
	m_compute.ubo.position = glm::vec4(m_scene->camera.eye, 1.0f);
	m_compute.ubo.forward = glm::vec4(m_scene->camera.forward, 0.0f);
	m_compute.ubo.up = glm::vec4(m_scene->camera.up, 0.0f);
	m_compute.ubo.right = glm::vec4(m_scene->camera.right, 0.0f);
	m_compute.ubo.lookat = glm::vec4(m_scene->camera.lookAt, 0.0f);

	m_vulkanDevice->MapMemory(
		&m_compute.ubo,
		m_compute.buffers.stagingUniform.memory,
		sizeof(m_compute.ubo),
		0
	);

	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.uniform.buffer,
		m_compute.buffers.stagingUniform.buffer,
		sizeof(m_compute.ubo));
}

void
VulkanGPURaytracer::Render() {
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

	// -- Submit compute command
	vkWaitForFences(m_vulkanDevice->device, 1, &m_compute.fence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_vulkanDevice->device, 1, &m_compute.fence);

	VkSubmitInfo computeSubmitInfo = MakeSubmitInfo(
		m_compute.commandBuffer
	);

	CheckVulkanResult(
		vkQueueSubmit(m_compute.queue, 1, &computeSubmitInfo, m_compute.fence),
		"Failed to submit queue"
	);
}

VulkanGPURaytracer::~VulkanGPURaytracer() {

	// Flush device to make sure all resources can be freed 
	vkDeviceWaitIdle(m_vulkanDevice->device);

	vkFreeCommandBuffers(m_vulkanDevice->device, m_compute.commandPool, 1, &m_compute.commandBuffer);
	vkDestroyCommandPool(m_vulkanDevice->device, m_compute.commandPool, nullptr);

	vkDestroyDescriptorSetLayout(m_vulkanDevice->device, m_compute.descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_vulkanDevice->device, m_compute.descriptorPool, nullptr);

	vkDestroyFence(m_vulkanDevice->device, m_compute.fence, nullptr);

	vkDestroyImageView(m_vulkanDevice->device, m_compute.storageRaytraceImage.imageView, nullptr);
	vkDestroyImage(m_vulkanDevice->device, m_compute.storageRaytraceImage.image, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.storageRaytraceImage.imageMemory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_compute.buffers.uniform.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.buffers.uniform.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_compute.buffers.stagingUniform.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.buffers.stagingUniform.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_compute.buffers.materials.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.buffers.materials.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_compute.buffers.indices.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.buffers.indices.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_compute.buffers.verticePositions.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_compute.buffers.verticePositions.memory, nullptr);

}

void VulkanGPURaytracer::PrepareGraphics() {
	PrepareGraphicsVertexBuffer();
	PrepareGraphicsDescriptorPool();
	PrepareGraphicsDescriptorSetLayout();
	PrepareGraphicsDescriptorSets();
	PrepareGraphicsPipeline();
	PrepareGraphicsCommandBuffers();
}

VkResult
VulkanGPURaytracer::PrepareGraphicsVertexBuffer() {
	m_graphics.geometryBuffers.clear();

	const std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3
	};

	const std::vector<vec2> positions = {
		{-1.0, -1.0},
		{1.0, -1.0},
		{1.0, 1.0},
		{-1.0, 1.0},
	};

	const std::vector<vec2> uvs = {
		{0.0, 0.0},
		{1.0, 0.0},
		{1.0, 1.0},
		{0.0, 1.0},
	};

	m_quad.indices = indices;
	m_quad.positions = positions;
	m_quad.uvs = uvs;

	VulkanBuffer::GeometryBuffer geomBuffer;

	// ----------- Vertex attributes --------------

	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
	VkDeviceSize indexBufferOffset = 0;
	VkDeviceSize positionBufferSize = sizeof(positions[0]) * positions.size();
	VkDeviceSize positionBufferOffset = indexBufferSize;
	VkDeviceSize uvBufferSize = sizeof(uvs[0]) * uvs.size();
	VkDeviceSize uvBufferOffset = positionBufferOffset + positionBufferSize;

	VkDeviceSize bufferSize = indexBufferSize + positionBufferSize + uvBufferSize;
	geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(INDEX, indexBufferOffset));
	geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(POSITION, positionBufferOffset));
	geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(TEXCOORD, uvBufferOffset));

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
	memcpy((Byte*)data, (Byte*)indices.data(), static_cast<size_t>(indexBufferSize));
	memcpy((Byte*)data + positionBufferOffset, (Byte*)positions.data(), static_cast<size_t>(positionBufferSize));
	memcpy((Byte*)data + uvBufferOffset, (Byte*)uvs.data(), static_cast<size_t>(uvBufferSize));
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

	return VK_SUCCESS;
}

VkResult
VulkanGPURaytracer::PrepareGraphicsDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes = {
		// Image sampler
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(
		poolSizes.size(),
		poolSizes.data(),
		3
	);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_graphics.descriptorPool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

VkResult
VulkanGPURaytracer::PrepareGraphicsDescriptorSetLayout() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: Fragment shader image sampler
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT
		),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
			MakeDescriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size()
			);

	CheckVulkanResult(
		vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_graphics.descriptorSetLayout),
		"Failed to create descriptor set layout"
	);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_graphics.descriptorSetLayout);

	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_graphics.pipelineLayout),
		"Failed to create pipeline layout"
	);

	return VK_SUCCESS;
}

VkResult
VulkanGPURaytracer::PrepareGraphicsDescriptorSets() {
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_graphics.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &descriptorSetAllocInfo, &m_graphics.descriptorSets),
		"failed to allocate descriptor sets"
	);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets =
	{
		// Binding 0 : Fragment shader texture sampler
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_graphics.descriptorSets,
			0, // binding
			1, // descriptor count
			nullptr, // buffer info
			&m_compute.storageRaytraceImage.descriptor // image info
		)
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	return VK_SUCCESS;
}

VkResult
VulkanGPURaytracer::PrepareGraphicsPipeline() {
	VkResult result = VK_SUCCESS;

	// Load SPIR-V bytecode
	// The SPIR_V files can be compiled by running glsllangValidator.exe from the VulkanSDK or
	// by invoking the custom script shaders/compileShaders.bat
	VkShaderModule vertShader;
	PrepareShaderModule("shaders/raytracing/raytrace.vert.spv", vertShader);
	VkShaderModule fragShader;
	PrepareShaderModule("shaders/raytracing/raytrace.frag.spv", fragShader);

	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineVertexInputStateCreateInfo
	// 1. Vertex input stage
	std::vector<VkVertexInputBindingDescription> bindingDesc = {
		MakeVertexInputBindingDescription(
			0, // binding
			sizeof(m_quad.positions[0]), // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		),
		MakeVertexInputBindingDescription(
			1, // binding
			sizeof(m_quad.uvs[0]), // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		)
	};

	// Attribute description (position, normal, texcoord etc.)
	std::vector<VkVertexInputAttributeDescription> attribDesc = {
		MakeVertexInputAttributeDescription(
			0, // binding
			0, // location
			VK_FORMAT_R32G32_SFLOAT,
			0 // offset
		),
		MakeVertexInputAttributeDescription(
			1, // binding
			1, // location
			VK_FORMAT_R32G32_SFLOAT,
			0 // offset
		)
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = MakePipelineVertexInputStateCreateInfo(
		bindingDesc,
		attribDesc
	);

	// 2. Input assembly
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineInputAssemblyStateCreateInfo
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
			MakePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// 3. Skip tesselation 
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineTessellationStateCreateInfo


	// 4. Viewports and scissors
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

	// 5. Rasterizer
	// This stage converts primitives into fragments. It also performs depth/stencil testing, face culling, scissor test.
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineRasterizationStateCreateInfo 
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = MakePipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	// 6. Multisampling state. We're not doing anything special here for now
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineMultisampleStateCreateInfo

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
			MakePipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	// 6. Depth/stecil tests state
	// \see https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#VkPipelineDepthStencilStateCreateInfo
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = MakePipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

	// 7. Color blending state
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

	// 8. Dynamic state. Some pipeline states can be updated dynamically. 


	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStateEnables.size();
	dynamicState.pDynamicStates = dynamicStateEnables.data();

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
VulkanGPURaytracer::PrepareGraphicsCommandBuffers() {
	m_graphics.commandBuffers.resize(m_vulkanDevice->m_swapchain.framebuffers.size());
	// Primary means that can be submitted to a queue, but cannot be called from other command buffers
	VkCommandBufferAllocateInfo allocInfo = MakeCommandBufferAllocateInfo(m_graphics.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vulkanDevice->m_swapchain.framebuffers.size());

	VkResult result = vkAllocateCommandBuffers(m_vulkanDevice->device, &allocInfo, m_graphics.commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command buffers.");
		return result;
	}

	for (int i = 0; i < m_graphics.commandBuffers.size(); ++i) {
		// Begin command recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_graphics.commandBuffers[i], &beginInfo);

		// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = m_compute.storageRaytraceImage.image;
		imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(
			m_graphics.commandBuffers[i],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		// Begin renderpass
		std::vector<VkClearValue> clearValues(2);
		glm::vec4 clearColor = NormalizeColor(0, 67, 100, 255);
		clearValues[0].color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
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

		VkViewport viewport = MakeFullscreenViewport(m_vulkanDevice->m_swapchain.extent);
		vkCmdSetViewport(m_graphics.commandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = m_vulkanDevice->m_swapchain.extent;
		vkCmdSetScissor(m_graphics.commandBuffers[i], 0, 1, &scissor);

		for (int b = 0; b < m_graphics.geometryBuffers.size(); ++b) {
			VulkanBuffer::GeometryBuffer& geomBuffer = m_graphics.geometryBuffers[b];

			// Bind vertex buffer
			VkBuffer vertexBuffers[] = {geomBuffer.vertexBuffer, geomBuffer.vertexBuffer};
			VkDeviceSize offsets[] = {geomBuffer.bufferLayout.vertexBufferOffsets.at(POSITION), geomBuffer.bufferLayout.vertexBufferOffsets.at(TEXCOORD)};
			vkCmdBindVertexBuffers(m_graphics.commandBuffers[i], 0, 2, vertexBuffers, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(m_graphics.commandBuffers[i], geomBuffer.vertexBuffer, geomBuffer.bufferLayout.vertexBufferOffsets.at(INDEX), VK_INDEX_TYPE_UINT16);

			// Bind uniform buffer
			vkCmdBindDescriptorSets(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.pipelineLayout, 0, 1, &m_graphics.descriptorSets, 0, nullptr);

			// Record draw command for the triangle!
			vkCmdDrawIndexed(m_graphics.commandBuffers[i], m_quad.indices.size(), 1, 0, 0, 0);
		}

		// Record end renderpass
		vkCmdEndRenderPass(m_graphics.commandBuffers[i]);

		// End command recording
		result = vkEndCommandBuffer(m_graphics.commandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffers");
			return result;
		}
	}

	return result;
}

// ===========================================================================================
//
// COMPUTE (for raytracing)
//
// ===========================================================================================

void
VulkanGPURaytracer::PrepareCompute() {
	vkGetDeviceQueue(m_vulkanDevice->device, m_vulkanDevice->queueFamilyIndices.computeFamily, 0, &m_compute.queue);

	PrepareComputeCommandPool();
	PrepareRayTraceTextureResources();
	PrepareComputeStorageBuffer();
	PrepareComputeUniformBuffer();
	PrepareComputeDescriptors();
	PrepareComputePipeline();
	PrepareComputeCommandBuffers();
}

void
VulkanGPURaytracer::PrepareComputeDescriptors() {
	// 2. Create descriptor set layout

	std::vector<VkDescriptorPoolSize> poolSizes = {
		// Output storage image of ray traced result
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
		// Uniform buffer for compute
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
		// Mesh storage buffers
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(
		poolSizes.size(),
		poolSizes.data(),
		5
	);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_compute.descriptorPool),
		"Failed to create descriptor pool"
	);

	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: output storage image
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 1: uniform buffer for compute
		MakeDescriptorSetLayoutBinding(
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 2: storage buffer for triangle indices
		MakeDescriptorSetLayoutBinding(
			2,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 3: storage buffer for triangle positions
		MakeDescriptorSetLayoutBinding(
			3,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 4: storage buffer for triangle normals
		MakeDescriptorSetLayoutBinding(
			4,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 5: uniform buffer for materials
		MakeDescriptorSetLayoutBinding(
			5,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
			MakeDescriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size()
			);

	CheckVulkanResult(
		vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_compute.descriptorSetLayout),
		"Failed to create descriptor set layout"
	);

	// 3. Allocate descriptor set

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = MakeDescriptorSetAllocateInfo(m_compute.descriptorPool, &m_compute.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &descriptorSetAllocInfo, &m_compute.descriptorSets),
		"failed to allocate descriptor set"
	);

	// 4. Update descriptor sets

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0, output storage image
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			m_compute.descriptorSets,
			0, // Binding 0
			1,
			nullptr,
			&m_compute.storageRaytraceImage.descriptor
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_compute.descriptorSets,
			1, // Binding 1
			1,
			&m_compute.buffers.uniform.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_compute.descriptorSets,
			2, // Binding 2
			1,
			&m_compute.buffers.indices.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_compute.descriptorSets,
			3, // Binding 3
			1,
			&m_compute.buffers.verticePositions.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_compute.descriptorSets,
			4, // Binding 4
			1,
			&m_compute.buffers.verticeNormals.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_compute.descriptorSets,
			5, // Binding 5
			1,
			&m_compute.buffers.materials.descriptor,
			nullptr
		),
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

}

void
VulkanGPURaytracer::PrepareComputeCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = MakeCommandPoolCreateInfo(m_vulkanDevice->queueFamilyIndices.computeFamily);

	CheckVulkanResult(
		vkCreateCommandPool(m_vulkanDevice->device, &commandPoolCreateInfo, nullptr, &m_compute.commandPool),
		"Failed to create command pool for compute"
	);
}

// SSBO plane declaration
struct Plane {
	glm::vec3 normal;
	float distance;
	glm::vec3 diffuse;
	float specular;
	uint32_t id;
	glm::ivec3 _pad;
};

uint32_t currentId = 0; // Id used to identify objects by the ray tracing shader

void
VulkanGPURaytracer::PrepareComputeStorageBuffer() {
	// =========== INDICES
	VulkanBuffer::StorageBuffer stagingBuffer;
	VkDeviceSize bufferSize = m_scene->indices.size() * sizeof(ivec4);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer,
		stagingBuffer.memory
	);

	m_vulkanDevice->MapMemory(
		m_scene->indices.data(),
		stagingBuffer.memory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.buffers.indices.buffer,
		m_compute.buffers.indices.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.indices.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_compute.buffers.indices.descriptor = MakeDescriptorBufferInfo(m_compute.buffers.indices.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

	// =========== VERTICE POSITIONS
	bufferSize = m_scene->verticePositions.size() * sizeof(glm::vec4);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer,
		stagingBuffer.memory
	);

	m_vulkanDevice->MapMemory(
		m_scene->verticePositions.data(),
		stagingBuffer.memory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.buffers.verticePositions.buffer,
		m_compute.buffers.verticePositions.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.verticePositions.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_compute.buffers.verticePositions.descriptor = MakeDescriptorBufferInfo(m_compute.buffers.verticePositions.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

	// =========== VERTICE NORMALS
	bufferSize = m_scene->verticeNormals.size() * sizeof(glm::vec4);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer,
		stagingBuffer.memory
	);

	m_vulkanDevice->MapMemory(
		m_scene->verticeNormals.data(),
		stagingBuffer.memory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.buffers.verticeNormals.buffer,
		m_compute.buffers.verticeNormals.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.verticeNormals.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_compute.buffers.verticeNormals.descriptor = MakeDescriptorBufferInfo(m_compute.buffers.verticeNormals.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

}

void VulkanGPURaytracer::PrepareComputeUniformBuffer() {
	// Initialize camera's ubo
	//calculate fov based on resolution
	float yscaled = tan(m_compute.ubo.fov * (glm::pi<float>() / 180.0f));
	float xscaled = (yscaled * m_vulkanDevice->m_swapchain.aspectRatio);

	m_compute.ubo.forward = glm::normalize(m_compute.ubo.lookat - m_compute.ubo.position);
	m_compute.ubo.pixelLength = glm::vec2(2 * xscaled / (float)m_vulkanDevice->m_swapchain.extent.width
	                                      , 2 * yscaled / (float)m_vulkanDevice->m_swapchain.extent.height);

	m_compute.ubo.aspectRatio = (float)m_vulkanDevice->m_swapchain.aspectRatio;


	VkDeviceSize bufferSize = sizeof(m_compute.ubo);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_compute.buffers.stagingUniform.buffer,
		m_compute.buffers.stagingUniform.memory
	);

	m_vulkanDevice->MapMemory(
		&m_compute.ubo,
		m_compute.buffers.stagingUniform.memory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.buffers.uniform.buffer,
		m_compute.buffers.uniform.memory
	);

	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.uniform.buffer,
		m_compute.buffers.stagingUniform.buffer,
		bufferSize
	);

	m_compute.buffers.uniform.descriptor = MakeDescriptorBufferInfo(m_compute.buffers.uniform.buffer, 0, bufferSize);

	// ====== MATERIALS
	bufferSize = sizeof(MaterialPacked) * m_scene->materials.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;


	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingMemory
	);

	m_vulkanDevice->MapMemory(
		m_scene->materials.data(),
		stagingMemory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.buffers.materials.buffer,
		m_compute.buffers.materials.memory
	);

	m_vulkanDevice->CopyBuffer(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.buffers.materials.buffer,
		stagingBuffer,
		bufferSize
	);

	m_compute.buffers.materials.descriptor = MakeDescriptorBufferInfo(m_compute.buffers.materials.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingMemory, nullptr);

}

VkResult
VulkanGPURaytracer::PrepareRayTraceTextureResources() {
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

	m_vulkanDevice->CreateImage(
		m_vulkanDevice->m_swapchain.extent.width,
		m_vulkanDevice->m_swapchain.extent.height,
		1, // only a 2D depth image
		VK_IMAGE_TYPE_2D,
		imageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		// Image is sampled in fragment shader and used as storage for compute output
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_compute.storageRaytraceImage.image,
		m_compute.storageRaytraceImage.imageMemory
	);
	m_vulkanDevice->CreateImageView(
		m_compute.storageRaytraceImage.image,
		VK_IMAGE_VIEW_TYPE_2D,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_compute.storageRaytraceImage.imageView
	);

	m_vulkanDevice->TransitionImageLayout(
		m_compute.queue,
		m_compute.commandPool,
		m_compute.storageRaytraceImage.image,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL
	);

	// Create sampler
	CreateDefaultImageSampler(m_vulkanDevice->device, &m_compute.storageRaytraceImage.sampler);

	m_compute.storageRaytraceImage.width = m_vulkanDevice->m_swapchain.extent.width;
	m_compute.storageRaytraceImage.height = m_vulkanDevice->m_swapchain.extent.height;

	// Initialize descriptor
	m_compute.storageRaytraceImage.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	m_compute.storageRaytraceImage.descriptor.imageView = m_compute.storageRaytraceImage.imageView;
	m_compute.storageRaytraceImage.descriptor.sampler = m_compute.storageRaytraceImage.sampler;

	return VK_SUCCESS;
}

VkResult
VulkanGPURaytracer::PrepareComputePipeline() {

	// 5. Create pipeline layout

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_compute.descriptorSetLayout);
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_compute.pipelineLayout),
		"Failed to create pipeline layout"
	);

	// 6. Create compute shader pipeline
	VkComputePipelineCreateInfo computePipelineCreateInfo = MakeComputePipelineCreateInfo(m_compute.pipelineLayout, 0);

	// Create shader modules from bytecodes
	VkShaderModule raytraceShader;
	PrepareShaderModule(
		"shaders/raytracing/raytrace.comp.spv",
		raytraceShader
	);
	m_logger->info("Loaded {} comp shader", "shaders/raytracing/raytrace.comp.spv");


	computePipelineCreateInfo.stage = MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, raytraceShader);

	CheckVulkanResult(
		vkCreateComputePipelines(m_vulkanDevice->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_compute.pipeline),
		"Failed to create compute pipeline"
	);


	// 7. Create fence
	VkFenceCreateInfo fenceCreateInfo = MakeFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	CheckVulkanResult(
		vkCreateFence(m_vulkanDevice->device, &fenceCreateInfo, nullptr, &m_compute.fence),
		"Failed to create fence"
	);

	return VK_SUCCESS;
}


VkResult
VulkanGPURaytracer::PrepareComputeCommandBuffers() {

	VkCommandBufferAllocateInfo commandBufferAllocInfo = MakeCommandBufferAllocateInfo(m_compute.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	CheckVulkanResult(
		vkAllocateCommandBuffers(m_vulkanDevice->device, &commandBufferAllocInfo, &m_compute.commandBuffer),
		"Failed to allocate compute command buffers"
	);

	// Begin command recording
	VkCommandBufferBeginInfo beginInfo = MakeCommandBufferBeginInfo();

	vkBeginCommandBuffer(m_compute.commandBuffer, &beginInfo);

	// Record binding to the compute pipeline
	vkCmdBindPipeline(m_compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute.pipeline);

	// Bind descriptor sets
	vkCmdBindDescriptorSets(m_compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute.pipelineLayout, 0, 1, &m_compute.descriptorSets, 0, nullptr);

	vkCmdDispatch(m_compute.commandBuffer, m_compute.storageRaytraceImage.width / 16, m_compute.storageRaytraceImage.height / 16, 1);

	CheckVulkanResult(
		vkEndCommandBuffer(m_compute.commandBuffer),
		"Failed to record command buffers"
	);

	return VK_SUCCESS;
}
