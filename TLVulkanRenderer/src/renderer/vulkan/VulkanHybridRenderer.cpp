#include "VulkanHybridRenderer.h"
#include "Utilities.h"
#include "scene/Camera.h"

VulkanHybridRenderer::VulkanHybridRenderer(
	GLFWwindow* window,
	Scene* scene,
	std::shared_ptr<std::map<string, string>> config
) : VulkanRenderer(window, scene, config) {

	PrepareComputeRaytrace();
	Prepare();
}

void
VulkanHybridRenderer::Update() {
	// Update camera ubo
	m_raytrace.ubo.position = glm::vec4(m_scene->camera.eye, 1.0f);
	m_raytrace.ubo.forward = glm::vec4(m_scene->camera.forward, 0.0f);
	m_raytrace.ubo.up = glm::vec4(m_scene->camera.up, 0.0f);
	m_raytrace.ubo.right = glm::vec4(m_scene->camera.right, 0.0f);
	m_raytrace.ubo.lookat = glm::vec4(m_scene->camera.lookAt, 0.0f);

	m_vulkanDevice->MapMemory(
		&m_raytrace.ubo,
		m_raytrace.buffers.stagingUniform.memory,
		sizeof(m_raytrace.ubo),
		0
	);

	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.uniform.buffer,
		m_raytrace.buffers.stagingUniform.buffer,
		sizeof(m_raytrace.ubo));
}

void
VulkanHybridRenderer::Render() {
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
	std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphore };
	std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphore };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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
	std::vector<VkSwapchainKHR> swapchains = { m_vulkanDevice->m_swapchain.swapchain };
	VkPresentInfoKHR presentInfo = MakePresentInfoKHR(
		signalSemaphores,
		swapchains,
		&imageIndex
	);

	vkQueuePresentKHR(m_graphics.queue, &presentInfo);

	// -- Submit compute command
	vkWaitForFences(m_vulkanDevice->device, 1, &m_raytrace.fence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_vulkanDevice->device, 1, &m_raytrace.fence);

	VkSubmitInfo computeSubmitInfo = MakeSubmitInfo(
		m_raytrace.commandBuffer
	);

	CheckVulkanResult(
		vkQueueSubmit(m_raytrace.queue, 1, &computeSubmitInfo, m_raytrace.fence),
		"Failed to submit queue"
	);
}

VulkanHybridRenderer::~VulkanHybridRenderer() {

	// Flush device to make sure all resources can be freed 
	vkDeviceWaitIdle(m_vulkanDevice->device);

	vkFreeCommandBuffers(m_vulkanDevice->device, m_raytrace.commandPool, 1, &m_raytrace.commandBuffer);
	vkDestroyCommandPool(m_vulkanDevice->device, m_raytrace.commandPool, nullptr);

	vkDestroyDescriptorSetLayout(m_vulkanDevice->device, m_raytrace.descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_vulkanDevice->device, m_raytrace.descriptorPool, nullptr);

	vkDestroyFence(m_vulkanDevice->device, m_raytrace.fence, nullptr);

	vkDestroyImageView(m_vulkanDevice->device, m_raytrace.storageRaytraceImage.imageView, nullptr);
	vkDestroyImage(m_vulkanDevice->device, m_raytrace.storageRaytraceImage.image, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.storageRaytraceImage.imageMemory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.uniform.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.uniform.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.stagingUniform.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.stagingUniform.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.materials.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.materials.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.indices.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.indices.memory, nullptr);

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.verticePositions.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.verticePositions.memory, nullptr);

}

void VulkanHybridRenderer::Prepare() {
	PrepareVertexBuffers();
	PrepareDescriptorPool();
	PrepareDescriptorLayouts();
	PrepareDescriptorSets();
	PreparePipelines();
	BuildCommandBuffers();
}

VkResult
VulkanHybridRenderer::PrepareVertexBuffers() {
	m_graphics.geometryBuffers.clear();

	const std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3
	};

	const std::vector<vec2> positions = {
		{ -1.0, -1.0 },
		{ 1.0, -1.0 },
		{ 1.0, 1.0 },
		{ -1.0, 1.0 },
	};

	const std::vector<vec2> uvs = {
		{ 0.0, 0.0 },
		{ 1.0, 0.0 },
		{ 1.0, 1.0 },
		{ 0.0, 1.0 },
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
VulkanHybridRenderer::PrepareDescriptorPool() {
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
VulkanHybridRenderer::PrepareDescriptorLayouts() {
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
VulkanHybridRenderer::PrepareDescriptorSets() {
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
			&m_raytrace.storageRaytraceImage.descriptor // image info
		)
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	return VK_SUCCESS;
}

VkResult
VulkanHybridRenderer::PreparePipelines() {
	VkResult result = VK_SUCCESS;

	// Load SPIR-V bytecode
	// The SPIR_V files can be compiled by running glsllangValidator.exe from the VulkanSDK or
	// by invoking the custom script shaders/compileShaders.bat
	VkShaderModule vertShader = MakeShaderModule(m_vulkanDevice->device, "shaders/raytracing/raytrace.vert.spv");
	VkShaderModule fragShader = MakeShaderModule(m_vulkanDevice->device, "shaders/raytracing/raytrace.frag.spv");

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
	scissor.offset = { 0, 0 };
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
VulkanHybridRenderer::BuildCommandBuffers() {
	m_graphics.commandBuffers.resize(m_vulkanDevice->m_swapchain.framebuffers.size());
	// Primary means that can be submitted to a queue, but cannot be called from other command buffers
	VkCommandBufferAllocateInfo allocInfo = MakeCommandBufferAllocateInfo(m_graphics.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vulkanDevice->m_swapchain.framebuffers.size());

	VkResult result = vkAllocateCommandBuffers(m_vulkanDevice->device, &allocInfo, m_graphics.commandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command buffers.");
		return result;
	}

	for (int i = 0; i < m_graphics.commandBuffers.size(); ++i)
	{
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
		imageMemoryBarrier.image = m_raytrace.storageRaytraceImage.image;
		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
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
		clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassBeginInfo = MakeRenderPassBeginInfo(
			m_graphics.renderPass,
			m_vulkanDevice->m_swapchain.framebuffers[i],
			{ 0, 0 },
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
		scissor.offset = { 0, 0 };
		scissor.extent = m_vulkanDevice->m_swapchain.extent;
		vkCmdSetScissor(m_graphics.commandBuffers[i], 0, 1, &scissor);

		for (int b = 0; b < m_graphics.geometryBuffers.size(); ++b)
		{
			VulkanBuffer::GeometryBuffer& geomBuffer = m_graphics.geometryBuffers[b];

			// Bind vertex buffer
			VkBuffer vertexBuffers[] = { geomBuffer.vertexBuffer, geomBuffer.vertexBuffer };
			VkDeviceSize offsets[] = { geomBuffer.bufferLayout.vertexBufferOffsets.at(POSITION), geomBuffer.bufferLayout.vertexBufferOffsets.at(TEXCOORD) };
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
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffers");
			return result;
		}
	}

	return result;
}

void 
VulkanHybridRenderer::CreateAttachment(
	VkFormat format, 
	VkImageUsageFlagBits usage, 
	VulkanImage::Image& attachment
	)
{
	VkImageAspectFlags aspectMask = 0;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	attachment = VulkanImage::CreateVulkanImage(
		m_vulkanDevice,
		m_deferred.framebuffer.width,
		m_deferred.framebuffer.height,
		VK_IMAGE_TILING_OPTIMAL,
		usage | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	attachment.format = format;
	m_vulkanDevice->CreateImageView(
		attachment.image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		aspectMask,
		attachment.imageView
	);
}
void VulkanHybridRenderer::PrepareDeferredAttachments()
{
	m_deferred.framebuffer.width = 2048;
	m_deferred.framebuffer.height = 2048;

	// World space positions
	CreateAttachment(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		m_deferred.framebuffer.position
	);

	// World space normals
	CreateAttachment(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		m_deferred.framebuffer.normal
	);

	// Albedo
	CreateAttachment(
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		m_deferred.framebuffer.albedo
	);


	// Depth attachment
	// Find a suitable depth format	
	VkFormat attDepthFormat = VulkanImage::FindDepthFormat(m_vulkanDevice->physicalDevice);
	
	CreateAttachment(
		attDepthFormat,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_deferred.framebuffer.depth
	);

	// Set up a new renderpass for the attachment
	std::array<VkAttachmentDescription, 4> attachmentDescs = {};

	// Init attachment properties
	for (uint8_t i = 0; i < 4; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// for depth
		if (i == 3)
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
	}

	// Formats
	attachmentDescs[0].format = m_deferred.framebuffer.position.format;
	attachmentDescs[1].format = m_deferred.framebuffer.normal.format;
	attachmentDescs[2].format = m_deferred.framebuffer.albedo.format;
	attachmentDescs[3].format = m_deferred.framebuffer.depth.format;

	// Attachment references
	std::array<VkAttachmentReference, 3> colorReferences;
	colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference depthReference = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	// Subpass description
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = colorReferences.size();
	subpass.pDepthStencilAttachment = &depthReference;

	// Subpass dependencies for attachment layout transition
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create deferred renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	CheckVulkanResult(vkCreateRenderPass(m_vulkanDevice->device, &renderPassInfo, nullptr, &m_deferred.framebuffer.renderPass), "Failed to create deferred render pass");
}

void
VulkanHybridRenderer::PrepareComputeRaytrace() {
	vkGetDeviceQueue(m_vulkanDevice->device, m_vulkanDevice->queueFamilyIndices.computeFamily, 0, &m_raytrace.queue);

	PrepareComputeRaytraceCommandPool();
	PrepareComputeRaytraceTextureResources();
	PrepareComputeRaytraceStorageBuffer();
	PrepareComputeRaytraceUniformBuffer();
	PrepareComputeRaytraceDescriptorSets();
	PrepareComputeRaytracePipeline();
	BuildComputeCommandBuffers();
}

void
VulkanHybridRenderer::PrepareComputeRaytraceDescriptorSets() {
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
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_raytrace.descriptorPool),
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
		vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_raytrace.descriptorSetLayout),
		"Failed to create descriptor set layout"
	);

	// 3. Allocate descriptor set

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = MakeDescriptorSetAllocateInfo(m_raytrace.descriptorPool, &m_raytrace.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &descriptorSetAllocInfo, &m_raytrace.descriptorSets),
		"failed to allocate descriptor set"
	);

	// 4. Update descriptor sets

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0, output storage image
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			m_raytrace.descriptorSets,
			0, // Binding 0
			1,
			nullptr,
			&m_raytrace.storageRaytraceImage.descriptor
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_raytrace.descriptorSets,
			1, // Binding 1
			1,
			&m_raytrace.buffers.uniform.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSets,
			2, // Binding 2
			1,
			&m_raytrace.buffers.indices.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSets,
			3, // Binding 3
			1,
			&m_raytrace.buffers.verticePositions.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSets,
			4, // Binding 4
			1,
			&m_raytrace.buffers.verticeNormals.descriptor,
			nullptr
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_raytrace.descriptorSets,
			5, // Binding 5
			1,
			&m_raytrace.buffers.materials.descriptor,
			nullptr
		),
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

}

void
VulkanHybridRenderer::PrepareComputeRaytraceCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = MakeCommandPoolCreateInfo(m_vulkanDevice->queueFamilyIndices.computeFamily);

	CheckVulkanResult(
		vkCreateCommandPool(m_vulkanDevice->device, &commandPoolCreateInfo, nullptr, &m_raytrace.commandPool),
		"Failed to create command pool for compute"
	);
}

// SSBO plane declaration
struct Plane
{
	glm::vec3 normal;
	float distance;
	glm::vec3 diffuse;
	float specular;
	uint32_t id;
	glm::ivec3 _pad;
};

void
VulkanHybridRenderer::PrepareComputeRaytraceStorageBuffer() {
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
		m_raytrace.buffers.indices.buffer,
		m_raytrace.buffers.indices.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.indices.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.indices.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.indices.buffer, 0, bufferSize);

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
		m_raytrace.buffers.verticePositions.buffer,
		m_raytrace.buffers.verticePositions.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.verticePositions.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.verticePositions.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.verticePositions.buffer, 0, bufferSize);

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
		m_raytrace.buffers.verticeNormals.buffer,
		m_raytrace.buffers.verticeNormals.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.verticeNormals.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.verticeNormals.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.verticeNormals.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

}

void VulkanHybridRenderer::PrepareComputeRaytraceUniformBuffer() {
	// Initialize camera's ubo
	//calculate fov based on resolution
	float yscaled = tan(m_raytrace.ubo.fov * (glm::pi<float>() / 180.0f));
	float xscaled = (yscaled * m_vulkanDevice->m_swapchain.aspectRatio);

	m_raytrace.ubo.forward = glm::normalize(m_raytrace.ubo.lookat - m_raytrace.ubo.position);
	m_raytrace.ubo.pixelLength = glm::vec2(2 * xscaled / (float)m_vulkanDevice->m_swapchain.extent.width
		, 2 * yscaled / (float)m_vulkanDevice->m_swapchain.extent.height);

	m_raytrace.ubo.aspectRatio = (float)m_vulkanDevice->m_swapchain.aspectRatio;


	VkDeviceSize bufferSize = sizeof(m_raytrace.ubo);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_raytrace.buffers.stagingUniform.buffer,
		m_raytrace.buffers.stagingUniform.memory
	);

	m_vulkanDevice->MapMemory(
		&m_raytrace.ubo,
		m_raytrace.buffers.stagingUniform.memory,
		bufferSize,
		0
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_raytrace.buffers.uniform.buffer,
		m_raytrace.buffers.uniform.memory
	);

	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.uniform.buffer,
		m_raytrace.buffers.stagingUniform.buffer,
		bufferSize
	);

	m_raytrace.buffers.uniform.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.uniform.buffer, 0, bufferSize);

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
		m_raytrace.buffers.materials.buffer,
		m_raytrace.buffers.materials.memory
	);

	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.materials.buffer,
		stagingBuffer,
		bufferSize
	);

	m_raytrace.buffers.materials.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.materials.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingMemory, nullptr);

}

VkResult
VulkanHybridRenderer::PrepareComputeRaytraceTextureResources() {
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
		m_raytrace.storageRaytraceImage.image,
		m_raytrace.storageRaytraceImage.imageMemory
	);
	m_vulkanDevice->CreateImageView(
		m_raytrace.storageRaytraceImage.image,
		VK_IMAGE_VIEW_TYPE_2D,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_raytrace.storageRaytraceImage.imageView
	);

	m_vulkanDevice->TransitionImageLayout(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.storageRaytraceImage.image,
		imageFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL
	);

	// Create sampler
	CreateDefaultImageSampler(m_vulkanDevice->device, &m_raytrace.storageRaytraceImage.sampler);

	m_raytrace.storageRaytraceImage.width = m_vulkanDevice->m_swapchain.extent.width;
	m_raytrace.storageRaytraceImage.height = m_vulkanDevice->m_swapchain.extent.height;

	// Initialize descriptor
	m_raytrace.storageRaytraceImage.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	m_raytrace.storageRaytraceImage.descriptor.imageView = m_raytrace.storageRaytraceImage.imageView;
	m_raytrace.storageRaytraceImage.descriptor.sampler = m_raytrace.storageRaytraceImage.sampler;

	return VK_SUCCESS;
}

VkResult
VulkanHybridRenderer::PrepareComputeRaytracePipeline() {

	// 5. Create pipeline layout

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_raytrace.descriptorSetLayout);
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_raytrace.pipelineLayout),
		"Failed to create pipeline layout"
	);

	// 6. Create compute shader pipeline
	VkComputePipelineCreateInfo computePipelineCreateInfo = MakeComputePipelineCreateInfo(m_raytrace.pipelineLayout, 0);

	// Create shader modules from bytecodes
	VkShaderModule raytraceShader = MakeShaderModule(m_vulkanDevice->device, "shaders/raytracing/raytrace.comp.spv");
	m_logger->info("Loaded {} comp shader", "shaders/raytracing/raytrace.comp.spv");


	computePipelineCreateInfo.stage = MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, raytraceShader);

	CheckVulkanResult(
		vkCreateComputePipelines(m_vulkanDevice->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_raytrace.pipeline),
		"Failed to create compute pipeline"
	);


	// 7. Create fence
	VkFenceCreateInfo fenceCreateInfo = MakeFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	CheckVulkanResult(
		vkCreateFence(m_vulkanDevice->device, &fenceCreateInfo, nullptr, &m_raytrace.fence),
		"Failed to create fence"
	);

	return VK_SUCCESS;
}


VkResult
VulkanHybridRenderer::BuildComputeCommandBuffers() {

	VkCommandBufferAllocateInfo commandBufferAllocInfo = MakeCommandBufferAllocateInfo(m_raytrace.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	CheckVulkanResult(
		vkAllocateCommandBuffers(m_vulkanDevice->device, &commandBufferAllocInfo, &m_raytrace.commandBuffer),
		"Failed to allocate compute command buffers"
	);

	// Begin command recording
	VkCommandBufferBeginInfo beginInfo = MakeCommandBufferBeginInfo();

	vkBeginCommandBuffer(m_raytrace.commandBuffer, &beginInfo);

	// Record binding to the compute pipeline
	vkCmdBindPipeline(m_raytrace.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytrace.pipeline);

	// Bind descriptor sets
	vkCmdBindDescriptorSets(m_raytrace.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytrace.pipelineLayout, 0, 1, &m_raytrace.descriptorSets, 0, nullptr);

	vkCmdDispatch(m_raytrace.commandBuffer, m_raytrace.storageRaytraceImage.width / 16, m_raytrace.storageRaytraceImage.height / 16, 1);

	CheckVulkanResult(
		vkEndCommandBuffer(m_raytrace.commandBuffer),
		"Failed to record command buffers"
	);

	return VK_SUCCESS;
}
