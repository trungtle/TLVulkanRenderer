#include "VulkanHybridRenderer.h"
#include "Utilities.h"
#include "scene/Camera.h"
#include "tinygltfloader/tiny_gltf_loader.h"
#include "accel/SBVH.h"

VulkanHybridRenderer::VulkanHybridRenderer(
	GLFWwindow* window,
	Scene* scene,
	std::shared_ptr<std::map<string, string>> config
) : VulkanRenderer(window, scene, config)
{
	m_deferred.commandBuffer = VK_NULL_HANDLE;
	Prepare();
}

void
VulkanHybridRenderer::Update() {
	m_deferred.mvpUnif.m_model = glm::mat4();
	m_deferred.mvpUnif.m_projection = m_scene->camera.GetProj();
	m_deferred.mvpUnif.m_view = m_scene->camera.GetView();

	m_vulkanDevice->MapMemory(
		&m_deferred.mvpUnif,
		m_deferred.buffers.mvpUnifStorage.memory,
		sizeof(m_deferred.mvpUnif)
	);
}

void
VulkanHybridRenderer::Render() {

	uint32_t imageIndex;

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

	// Acquire the swapchain
	vkAcquireNextImageKHR(
		m_vulkanDevice->device,
		m_vulkanDevice->m_swapchain.swapchain,
		UINT64_MAX, // Timeout
		m_imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&imageIndex
	);

	// The scene render command buffer has to wait for the offscreen
	// rendering to be finished before we can use the framebuffer 
	// color image for sampling during final rendering
	// To ensure this we use a dedicated offscreen synchronization
	// semaphore that will be signaled when offscreen renderin
	// has been finished
	// This is necessary as an implementation may start both
	// command buffers at the same time, there is no guarantee
	// that command buffers will be executed in the order they
	// have been submitted by the application

	// =====  Offscreen rendering

	// Wait for swap chain presentation to finish
	submitInfo.pWaitSemaphores = &m_imageAvailableSemaphore;
	// Signal ready with offscreen semaphore
	submitInfo.pSignalSemaphores = &m_deferred.semaphore;

	// Submit work
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_deferred.commandBuffer;
	CheckVulkanResult(vkQueueSubmit(m_graphics.queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit queue");

	// ==== Submit rendering out for final pass

	// Wait for offscreen semaphore
	submitInfo.pWaitSemaphores = &m_deferred.semaphore;
	// Signal ready with render complete semaphpre
	submitInfo.pSignalSemaphores = &m_renderFinishedSemaphore;

	submitInfo.pCommandBuffers = &m_graphics.commandBuffers[imageIndex];
	CheckVulkanResult(vkQueueSubmit(m_graphics.queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit queue");

	// Present swapchain image! Use the signal semaphore for present swapchain to wait for the next one
	std::vector<VkSwapchainKHR> swapchains = { m_vulkanDevice->m_swapchain.swapchain };
	VkPresentInfoKHR presentInfo = MakePresentInfoKHR(
		signalSemaphores,
		swapchains,
		&imageIndex
	);

	vkQueuePresentKHR(m_graphics.queue, &presentInfo);

}

VulkanHybridRenderer::~VulkanHybridRenderer() {

	m_deferred.buffers.mvpUnifStorage.Destroy();
	m_deferred.buffers.lightsUnifStorage.Destroy();

	// Flush device to make sure all resources can be freed 
	vkDeviceWaitIdle(m_vulkanDevice->device);

	vkFreeCommandBuffers(m_vulkanDevice->device, m_raytrace.commandPool, 1, &m_raytrace.commandBuffer);
	vkDestroyCommandPool(m_vulkanDevice->device, m_raytrace.commandPool, nullptr);

	vkDestroyDescriptorSetLayout(m_vulkanDevice->device, m_raytrace.descriptorSetLayout, nullptr);

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

	vkDestroyBuffer(m_vulkanDevice->device, m_raytrace.buffers.positions.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_raytrace.buffers.positions.memory, nullptr);

}

void VulkanHybridRenderer::Prepare() {
	PrepareVertexBuffers();
	PrepareDescriptorPool();

	PrepareDeferred();
	PrepareOnscreen();
	PrepareComputeRaytrace();
}

VkResult
VulkanHybridRenderer::PrepareVertexBuffers() {
	m_graphics.geometryBuffers.clear();

	// ----------- Vertex attributes --------------

	for (MeshData* geomData : m_scene->meshesData)
	{
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
		std::vector<Byte> uvData;
		VkDeviceSize uvBufferSize = 0;
		VkDeviceSize uvBufferOffset = 0;
		if (geomData->vertexData.find(TEXCOORD) != geomData->vertexData.end())
		{
			uvData = geomData->vertexData.at(TEXCOORD);
			uvBufferSize = sizeof(uvData[0]) * uvData.size();
			uvBufferOffset = normalBufferOffset + normalBufferSize;
		}

		VkDeviceSize bufferSize = indexBufferSize + positionBufferSize + normalBufferSize + uvBufferSize;
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(INDEX, indexBufferOffset));
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(POSITION, positionBufferOffset));
		geomBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(NORMAL, normalBufferOffset));
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
		memcpy(data, indexData.data(), static_cast<size_t>(indexBufferSize));
		memcpy((Byte*)data + positionBufferOffset, positionData.data(), static_cast<size_t>(positionBufferSize));
		memcpy((Byte*)data + normalBufferOffset, normalData.data(), static_cast<size_t>(normalBufferSize));
		memcpy((Byte*)data + uvBufferOffset, uvData.data(), static_cast<size_t>(uvBufferSize));
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
VulkanHybridRenderer::PrepareDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes = {
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12),
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 14),
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(
		poolSizes.size(),
		poolSizes.data(),
		5
	);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_graphics.descriptorPool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

void 
VulkanHybridRenderer::CreateAttachment(
	VkFormat format, 
	VkImageUsageFlagBits usage, 
	VulkanImage::Image& attachment
	) const {
	VkImageAspectFlags aspectMask = 0;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	attachment = VulkanImage::CreateVulkanImage(
		m_vulkanDevice,
		m_deferred.framebuffer.width,
		m_deferred.framebuffer.height,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		usage | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_vulkanDevice->CreateImageView(
		attachment.image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		aspectMask,
		attachment.imageView
	);
}

void VulkanHybridRenderer::PrepareDeferred() {
	PrepareTextures();
	PrepareDeferredUniformBuffer();
	PrepareDeferredDescriptorLayout();
	PrepareDeferredAttachments();
	PrepareDeferredDescriptorSet();
	PrepareDeferredPipeline();
	BuildDeferredCommandBuffer();
}

void VulkanHybridRenderer::PrepareOnscreen() 
{
	PrepareOnScreenQuadVertexBuffer();
	PrepareOnscreenUniformBuffer();
	PrepareOnscreenDescriptorLayout();
	PrepareOnscreenDescriptorSet();
	PrepareOnscreenPipeline();
	BuildOnscreenCommandBuffer();
}

void VulkanHybridRenderer::PrepareOnScreenQuadVertexBuffer() {
	// ------------- quad ------------- //

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

	// ----------- Vertex attributes --------------

	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
	VkDeviceSize indexBufferOffset = 0;
	VkDeviceSize positionBufferSize = sizeof(positions[0]) * positions.size();
	VkDeviceSize positionBufferOffset = indexBufferSize;
	VkDeviceSize uvBufferSize = sizeof(uvs[0]) * uvs.size();
	VkDeviceSize uvBufferOffset = positionBufferOffset + positionBufferSize;

	VkDeviceSize bufferSize = indexBufferSize + positionBufferSize + uvBufferSize;
	m_onscreen.quadBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(INDEX, indexBufferOffset));
	m_onscreen.quadBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(POSITION, positionBufferOffset));
	m_onscreen.quadBuffer.bufferLayout.vertexBufferOffsets.insert(std::make_pair(TEXCOORD, uvBufferOffset));

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
		m_onscreen.quadBuffer.vertexBuffer
	);

	// Allocate memory for the buffer
	m_vulkanDevice->CreateMemory(
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_onscreen.quadBuffer.vertexBuffer,
		m_onscreen.quadBuffer.vertexBufferMemory
	);

	// Bind buffer with memory
	vkBindBufferMemory(m_vulkanDevice->device, m_onscreen.quadBuffer.vertexBuffer, m_onscreen.quadBuffer.vertexBufferMemory, memoryOffset);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_graphics.queue,
		m_graphics.commandPool,
		m_onscreen.quadBuffer.vertexBuffer,
		stagingBuffer,
		bufferSize
	);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBufferMemory, nullptr);
}

void VulkanHybridRenderer::PrepareOnscreenDescriptorLayout() 
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT
		),
		MakeDescriptorSetLayoutBinding(
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_FRAGMENT_BIT
		),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
		MakeDescriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			setLayoutBindings.size()
		);

	VkResult result = vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_graphics.descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_graphics.descriptorSetLayout);
	pipelineLayoutCreateInfo.pSetLayouts = &m_graphics.descriptorSetLayout;
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_graphics.pipelineLayout),
		"Failed to create pipeline layout."
	);
}

void VulkanHybridRenderer::PrepareOnscreenDescriptorSet() {
	VkDescriptorSetAllocateInfo allocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_graphics.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &allocInfo, &m_graphics.descriptorSet),
		"Failed to allocate descriptor set"
	);

	std::vector<VkWriteDescriptorSet> descriptorWrites =
	{
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_graphics.descriptorSet,
			0,
			1,
			nullptr,
			&m_deferred.framebuffer.albedo.descriptor
		),
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_graphics.descriptorSet,
			1,
			1,
			&m_onscreen.unifBuffer.descriptor,
			nullptr
		),
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void VulkanHybridRenderer::PrepareOnscreenUniformBuffer() {
	
	// === Quad === //
	m_onscreen.unifBuffer.Create(
		m_vulkanDevice,
		sizeof(glm::ivec2),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_vulkanDevice->MapMemory(
		&m_scene->camera.resolution,
		m_onscreen.unifBuffer.memory,
		sizeof(glm::ivec2)
	);
}

void VulkanHybridRenderer::PrepareOnscreenPipeline() 
{
	VkResult result = VK_SUCCESS;

	VkShaderModule vertShader = MakeShaderModule(m_vulkanDevice->device, "shaders/raytracing/quad.vert.spv");
	VkShaderModule fragShader = MakeShaderModule(m_vulkanDevice->device, "shaders/raytracing/quad.frag.spv");

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

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		MakePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

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

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = MakePipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
		MakePipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = MakePipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {
		colorBlendAttachmentState
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = MakePipelineColorBlendStateCreateInfo(colorBlendAttachments);

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos = {
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	};

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
			&m_graphics.m_pipeline // Pipelines
		),
		"Failed to create graphics pipeline"
	);

	vkDestroyShaderModule(m_vulkanDevice->device, vertShader, nullptr);
	vkDestroyShaderModule(m_vulkanDevice->device, fragShader, nullptr);
}

void VulkanHybridRenderer::BuildOnscreenCommandBuffer() 
{
	m_graphics.commandBuffers.resize(m_vulkanDevice->m_swapchain.framebuffers.size());
	// Primary means that can be submitted to a queue, but cannot be called from other command buffers
	VkCommandBufferAllocateInfo allocInfo = MakeCommandBufferAllocateInfo(m_graphics.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vulkanDevice->m_swapchain.framebuffers.size());

	VkResult result = vkAllocateCommandBuffers(m_vulkanDevice->device, &allocInfo, m_graphics.commandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command buffers.");
	}

	for (int i = 0; i < m_graphics.commandBuffers.size(); ++i)
	{
		// Begin command recording
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_graphics.commandBuffers[i], &beginInfo);

		// Begin renderpass
		std::vector<VkClearValue> clearValues(2);
		glm::vec4 clearColor = NormalizeColor(0, 0, 0, 255);
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
		vkCmdBindPipeline(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.m_pipeline);

		VkViewport viewport = MakeFullscreenViewport(m_vulkanDevice->m_swapchain.extent);
		vkCmdSetViewport(m_graphics.commandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_vulkanDevice->m_swapchain.extent;
		vkCmdSetScissor(m_graphics.commandBuffers[i], 0, 1, &scissor);

		// Bind vertex buffer
		VkBuffer vertexBuffers[] = { m_onscreen.quadBuffer.vertexBuffer, m_onscreen.quadBuffer.vertexBuffer };
		VkDeviceSize offsets[] = { 0, 0 };
		vkCmdBindVertexBuffers(m_graphics.commandBuffers[i], 0, 2, vertexBuffers, offsets);

		// Bind index buffer
		vkCmdBindIndexBuffer(m_graphics.commandBuffers[i], m_onscreen.quadBuffer.vertexBuffer, m_onscreen.quadBuffer.bufferLayout.vertexBufferOffsets.at(INDEX), VK_INDEX_TYPE_UINT16);

		// Bind uniform buffer
		vkCmdBindDescriptorSets(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.pipelineLayout, 0, 1, &m_graphics.descriptorSet, 0, nullptr);

		// Record draw command for the triangle!
		vkCmdDrawIndexed(m_graphics.commandBuffers[i], m_quad.indices.size(), 1, 0, 0, 0);

		// Record end renderpass
		vkCmdEndRenderPass(m_graphics.commandBuffers[i]);

		// End command recording
		result = vkEndCommandBuffer(m_graphics.commandBuffers[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffers");
		}
	}

}

void VulkanHybridRenderer::PrepareDeferredAttachments()
{
	m_deferred.framebuffer.width = m_scene->camera.resolution.x;
	m_deferred.framebuffer.height = m_scene->camera.resolution.y;

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

	std::array<VkImageView, 4> attachments;
	attachments[0] = m_deferred.framebuffer.position.imageView;
	attachments[1] = m_deferred.framebuffer.normal.imageView;
	attachments[2] = m_deferred.framebuffer.albedo.imageView;
	attachments[3] = m_deferred.framebuffer.depth.imageView;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = m_deferred.framebuffer.renderPass;
	fbufCreateInfo.pAttachments = attachments.data();
	fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbufCreateInfo.width = m_deferred.framebuffer.width;
	fbufCreateInfo.height = m_deferred.framebuffer.height;
	fbufCreateInfo.layers = 1;
	CheckVulkanResult(vkCreateFramebuffer(m_vulkanDevice->device, &fbufCreateInfo, nullptr, &m_deferred.framebuffer.vkFrameBuffer), "Failed to create framebuffer");

	// Create sampler to sample from the color attachments
	VkSamplerCreateInfo samplerCreate = MakeSamplerCreateInfo(
		VK_FILTER_NEAREST,
		VK_FILTER_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f,
		0,
		0.0f,
		1.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
	);

	CheckVulkanResult(vkCreateSampler(m_vulkanDevice->device, &samplerCreate, nullptr, &m_deferred.framebuffer.sampler), "Failed to create sampler");

	m_deferred.framebuffer.position.sampler = m_deferred.framebuffer.sampler;
	m_deferred.framebuffer.normal.sampler = m_deferred.framebuffer.sampler;
	m_deferred.framebuffer.albedo.sampler = m_deferred.framebuffer.sampler;

	// Create image descriptors
	VkDescriptorImageInfo texDescriptorPosition = VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_deferred.framebuffer.position
	);

	VkDescriptorImageInfo texDescriptorNormal = VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_deferred.framebuffer.normal
	);

	VkDescriptorImageInfo texDescriptorAlbedo = VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_deferred.framebuffer.albedo
	);
}

void VulkanHybridRenderer::PrepareTextures()
{
	// Stage image
	m_stagingImage = VulkanImage::CreateVulkanImage(
		m_vulkanDevice,
		m_width,
		m_height,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout stagingImageLayout;
	vkGetImageSubresourceLayout(m_vulkanDevice->device, m_stagingImage.image, &subresource, &stagingImageLayout);


	// Map memory from film to screen
	void* data;

	for (auto m : m_scene->materials) {
		if (m->m_texture != nullptr && m->m_texture->hasRawByte()) {
			VkDeviceSize imageSize = m->m_texture->width() *  m->m_texture->width();

			vkMapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory, 0, imageSize, 0, &data);
			if (stagingImageLayout.rowPitch == m->m_texture->width() * 4)
			{
				memcpy(data, m->m_texture->getRawByte().data(), (size_t)imageSize);
			}
			else
			{
				uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

				for (int y = 0; y <  m->m_texture->height(); y++)
				{
					memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &m->m_texture->getRawByte().data()[y *  m->m_texture->width() * 4], m->m_texture->width() * 4);
				}
			}
			
			vkUnmapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory);
			break;
		}
	}

	// Prepare our texture for staging
	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_stagingImage.image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	// Create our display imageMakeDescriptorImageInfo
	m_deferred.textures.m_colorMap = VulkanImage::CreateVulkanImage(
		m_vulkanDevice,
		m_width,
		m_height,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_deferred.textures.m_colorMap.image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	m_vulkanDevice->CopyImage(m_graphics.queue, m_graphics.commandPool, m_deferred.textures.m_colorMap.image, m_stagingImage.image, m_width, m_height);

	// Create image view
	m_vulkanDevice->CreateImageView(
		m_deferred.textures.m_colorMap.image,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_deferred.textures.m_colorMap.imageView
	);

	// Create image sampler
	CreateDefaultImageSampler(m_vulkanDevice->device, &m_deferred.textures.m_colorMap.sampler);

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_deferred.textures.m_colorMap.image, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Set descriptor
	VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_deferred.textures.m_colorMap
	);

	VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_deferred.textures.m_normalMap
	);

}

void VulkanHybridRenderer::PrepareDeferredDescriptorLayout() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
	{
		// Binding 0 : Vertex shader uniform buffer
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT
			),
		// Binding 1 : Position texture target / Scene colormap
		MakeDescriptorSetLayoutBinding(
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT
			),
		// Binding 2 : Normals texture target
		MakeDescriptorSetLayoutBinding(
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT
			)
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayout =
		MakeDescriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			setLayoutBindings.size()
		);

	CheckVulkanResult(
		vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorLayout, nullptr, &m_deferred.descriptorSetLayout),
		"Failed to create descriptor set layout"
	);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
		MakePipelineLayoutCreateInfo(&m_deferred.descriptorSetLayout);

	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_deferred.pipelineLayout),
		"Failed to create pipeline layout"
	);

}

void VulkanHybridRenderer::PrepareDeferredDescriptorSet() {
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_deferred.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &descriptorSetAllocInfo, &m_deferred.descriptorSet),
		"failed to allocate descriptor sets"
	);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets =
	{
		// Binding 0: Vertex shader uniform buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_deferred.descriptorSet,
			0, // binding
			1, // descriptor count
			&m_deferred.buffers.mvpUnifStorage.descriptor, // buffer info
			nullptr // image info
		),
		// Binding 1: Color map
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_deferred.descriptorSet,
			1, // binding
			1, // descriptor count
			nullptr, // buffer info
			&m_deferred.textures.m_colorMap.descriptor // image info
		),
		//// Binding 2: Normal map
		//MakeWriteDescriptorSet(
		//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		//	m_deferred.descriptorSet,
		//	2, // binding
		//	1, // descriptor count
		//	nullptr, // buffer info
		//	&m_deferred.textures.m_normalMap.descriptor // image info
		//),
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

}

void VulkanHybridRenderer::PrepareDeferredUniformBuffer() 
{
	// Deferred vertex shader
	m_deferred.buffers.mvpUnifStorage.Create(
		m_vulkanDevice, 
		sizeof(m_deferred.mvpUnif), 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_deferred.buffers.lightsUnifStorage.Create(
		m_vulkanDevice,
		sizeof(m_deferred.buffers.lightsUnifStorage),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
}

void 
VulkanHybridRenderer::PrepareDeferredPipeline() 
{
	VkResult result = VK_SUCCESS;

	VkShaderModule vertShader = MakeShaderModule(m_vulkanDevice->device, "shaders/deferred/mrt.vert.spv");
	VkShaderModule fragShader = MakeShaderModule(m_vulkanDevice->device, "shaders/deferred/mrt.frag.spv");

	std::vector<VkVertexInputBindingDescription> bindingDesc = {
		MakeVertexInputBindingDescription(
			0, // binding
			sizeof(glm::vec3) , // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		),
		MakeVertexInputBindingDescription(
			1, // binding
			sizeof(glm::vec3), // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		),
		MakeVertexInputBindingDescription(
			2, // binding
			sizeof(glm::vec2), // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		)
	};

	std::vector<VkVertexInputAttributeDescription> attribDesc = {
		MakeVertexInputAttributeDescription(
			0, // binding
			0, // location
			VK_FORMAT_R32G32B32_SFLOAT,
			0 // offset
		),
		MakeVertexInputAttributeDescription(
			0, // binding
			1, // location
			VK_FORMAT_R32G32B32_SFLOAT,
			0 // offset
		),
		MakeVertexInputAttributeDescription(
			0, // binding
			2, // location
			VK_FORMAT_R32G32_SFLOAT,
			0 // offset
		)
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = MakePipelineVertexInputStateCreateInfo(
		bindingDesc,
		attribDesc
	);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		MakePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

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

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = MakePipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL, 
		VK_CULL_MODE_BACK_BIT, 
		VK_FRONT_FACE_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
		MakePipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = MakePipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	// Blend attachment states required for all color attachments
	// This is important, as color write mask will otherwise be 0x0 and you
	// won't see anything rendered to the attachment
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {
		colorBlendAttachmentState,
		colorBlendAttachmentState,
		colorBlendAttachmentState
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = MakePipelineColorBlendStateCreateInfo(colorBlendAttachments);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_deferred.descriptorSetLayout);
	pipelineLayoutCreateInfo.pSetLayouts = &m_deferred.descriptorSetLayout;
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_deferred.pipelineLayout),
		"Failed to create pipeline layout."
	);

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos = {
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
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
			m_deferred.pipelineLayout,
			m_deferred.framebuffer.renderPass,
			0, // Subpass
			VK_NULL_HANDLE,
			-1
		);

	CheckVulkanResult(
		vkCreateGraphicsPipelines(
			m_vulkanDevice->device,
			VK_NULL_HANDLE, // Pipeline caches here
			1, // Pipeline count
			&pipelineCreateInfo,
			nullptr,
			&m_deferred.pipeline // Pipelines
		),
		"Failed to create graphics pipeline"
	);


	vkDestroyShaderModule(m_vulkanDevice->device, vertShader, nullptr);
	vkDestroyShaderModule(m_vulkanDevice->device, fragShader, nullptr);
}

void VulkanHybridRenderer::BuildDeferredCommandBuffer()
{
	if (m_deferred.commandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo allocInfo = MakeCommandBufferAllocateInfo(m_graphics.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		CheckVulkanResult(
			vkAllocateCommandBuffers(m_vulkanDevice->device, &allocInfo, &m_deferred.commandBuffer),
			"Failed to create command buffers."
		); 
	}

	// Create a semaphore used to synchronize offscreen rendering and usage
	VkSemaphoreCreateInfo semaphoreCreateInfo = MakeSemaphoreCreateInfo();
	VkResult result = vkCreateSemaphore(m_vulkanDevice->device, &semaphoreCreateInfo, nullptr, &m_deferred.semaphore);

	VkCommandBufferBeginInfo cmdBufInfo = MakeCommandBufferBeginInfo();

	// Clear values for all attachments written in the fragment sahder
	std::vector<VkClearValue> clearValues(4);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	// Begin renderpassw
	VkRenderPassBeginInfo renderPassBeginInfo = MakeRenderPassBeginInfo(
		m_deferred.framebuffer.renderPass, 
		m_deferred.framebuffer.vkFrameBuffer, 
		{ 0, 0 }, 
		{ m_deferred.framebuffer.width, m_deferred.framebuffer.height }, 
		clearValues
	);

	CheckVulkanResult(vkBeginCommandBuffer(m_deferred.commandBuffer, &cmdBufInfo), "Failed to create command buffer");

	vkCmdBeginRenderPass(m_deferred.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = MakeFullscreenViewport({ m_deferred.framebuffer.width, m_deferred.framebuffer.height });
	vkCmdSetViewport(m_deferred.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = { m_deferred.framebuffer.width, m_deferred.framebuffer.height, 0, 0 };
	vkCmdSetScissor(m_deferred.commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(m_deferred.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferred.pipeline);

	// Object

	for (int b = 0; b < m_graphics.geometryBuffers.size(); ++b)
	{
		VulkanBuffer::GeometryBuffer& geomBuffer = m_graphics.geometryBuffers[b];

		// Bind vertex buffer
		std::vector<VkBuffer> vertexBuffers = { 
			geomBuffer.vertexBuffer, 
			geomBuffer.vertexBuffer, 
			geomBuffer.vertexBuffer 
		};
		std::vector<VkDeviceSize> offsets = { 
			geomBuffer.bufferLayout.vertexBufferOffsets.at(POSITION),
			geomBuffer.bufferLayout.vertexBufferOffsets.at(NORMAL),
			geomBuffer.bufferLayout.vertexBufferOffsets.at(TEXCOORD)
		};
		vkCmdBindVertexBuffers(m_deferred.commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());

		// Bind index buffer
		VkIndexType indexType = VK_INDEX_TYPE_UINT16;
		if (m_scene->meshesData[b]->attribInfo.at(INDEX).componentTypeByteSize == 4) {
			indexType = VK_INDEX_TYPE_UINT32;
		}

		vkCmdBindIndexBuffer(m_deferred.commandBuffer, geomBuffer.vertexBuffer, geomBuffer.bufferLayout.vertexBufferOffsets.at(INDEX), indexType);

		// Bind uniform buffer
		vkCmdBindDescriptorSets(m_deferred.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferred.pipelineLayout, 0, 1, &m_deferred.descriptorSet, 0, nullptr);

		// Record draw command for the triangle!
		vkCmdDrawIndexed(m_deferred.commandBuffer, m_scene->meshesData[b]->attribInfo.at(INDEX).count, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(m_deferred.commandBuffer);

	CheckVulkanResult(vkEndCommandBuffer(m_deferred.commandBuffer), "Failed to end renderpass");
}

void
VulkanHybridRenderer::PrepareComputeRaytrace() {
	PrepareComputeRaytraceCommandPool();
	PrepareComputeRaytraceTextures();
	PrepareComputeRaytraceStorageBuffer();
	PrepareComputeRaytraceUniformBuffer();
	PrepareComputeRaytraceDescriptorLayout();
	PrepareComputeRaytraceDescriptorSet();
	PrepareComputeRaytracePipeline();
	BuildComputeRaytraceCommandBuffer();
}

void VulkanHybridRenderer::PrepareComputeRaytraceDescriptorLayout() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0 :  Position storage image 
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 1 : Normal storage image
		MakeDescriptorSetLayoutBinding(
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 2 : result of raytracing image
		MakeDescriptorSetLayoutBinding(
			2,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 3 : triangle indices + materialID of index
		MakeDescriptorSetLayoutBinding(
			3,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 4 : triangle positions
		MakeDescriptorSetLayoutBinding(
			4,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 5 : triangle normals
		MakeDescriptorSetLayoutBinding(
			5,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 6 : ubo
		MakeDescriptorSetLayoutBinding(
			6,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),		
		// Binding 7 : materials
		MakeDescriptorSetLayoutBinding(
			7,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		),
		// Binding 8 : bvhAabbNodes buffer
		MakeDescriptorSetLayoutBinding(
			8,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT
		)
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

	// -- Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_raytrace.descriptorSetLayout);

	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_raytrace.pipelineLayout),
		"Failed to create pipeline layout"
	);
}

void
VulkanHybridRenderer::PrepareComputeRaytraceDescriptorSet() {

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_raytrace.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &descriptorSetAllocInfo, &m_raytrace.descriptorSet),
		"failed to allocate descriptor sets"
	);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0 : Positions storage image
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_raytrace.descriptorSet,
			0, // Binding 0
			1,
			nullptr,
			&m_deferred.framebuffer.position.descriptor
		),
		// Binding 1 : Normals storage image
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_raytrace.descriptorSet,
			1, // Binding 1
			1,
			nullptr,
			&m_deferred.framebuffer.normal.descriptor
		),
		// Binding 2 : Result of raytracing storage image
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			m_raytrace.descriptorSet,
			2, // Binding 2
			1,
			nullptr,
			&m_raytrace.storageRaytraceImage.descriptor
		),
		// Binding 3 : Index buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSet,
			3, // Binding 3
			1,
			&m_raytrace.buffers.indices.descriptor,
			nullptr
		),
		// Binding 4 : Position buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSet,
			4, // Binding 4
			1,
			&m_raytrace.buffers.positions.descriptor,
			nullptr
		),
		// Binding 5 : Normal buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSet,
			5, // Binding 5
			1,
			&m_raytrace.buffers.normals.descriptor,
			nullptr
		),
		// Binding 6 : UBO
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_raytrace.descriptorSet,
			6, // Binding 6
			1,
			&m_raytrace.buffers.uniform.descriptor,
			nullptr
		),
		// Binding 7 : Materials buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_raytrace.descriptorSet,
			7, // Binding 7
			1,
			&m_raytrace.buffers.materials.descriptor,
			nullptr
		),
		// Binding 8 : bvh buffer
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			m_raytrace.descriptorSet,
			8, // Binding 8
			1,
			&m_raytrace.buffers.bvh.descriptor,
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

	// Get device queue for compute
	vkGetDeviceQueue(m_vulkanDevice->device, m_vulkanDevice->queueFamilyIndices.computeFamily, 0, &m_raytrace.queue);
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
		m_raytrace.buffers.positions.buffer,
		m_raytrace.buffers.positions.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.positions.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.positions.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.positions.buffer, 0, bufferSize);

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
		bufferSize
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_raytrace.buffers.normals.buffer,
		m_raytrace.buffers.normals.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.normals.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.normals.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.normals.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

	// =========== BVH
	SBVH* sbvh = reinterpret_cast<SBVH*>(m_scene->m_accel.get());
	bufferSize = sbvh->m_nodes.size() * sizeof(SBVHNode);

	// Stage
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer,
		stagingBuffer.memory
	);

	m_vulkanDevice->MapMemory(
		sbvh->m_nodes.data(),
		stagingBuffer.memory,
		bufferSize
	);

	// -----------------------------------------

	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_raytrace.buffers.bvh.buffer,
		m_raytrace.buffers.bvh.memory
	);

	// Copy over to vertex buffer in device local memory
	m_vulkanDevice->CopyBuffer(
		m_raytrace.queue,
		m_raytrace.commandPool,
		m_raytrace.buffers.bvh.buffer,
		stagingBuffer.buffer,
		bufferSize
	);

	m_raytrace.buffers.bvh.descriptor = MakeDescriptorBufferInfo(m_raytrace.buffers.bvh.buffer, 0, bufferSize);

	// Cleanup staging buffer memory
	vkDestroyBuffer(m_vulkanDevice->device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(m_vulkanDevice->device, stagingBuffer.memory, nullptr);

}

void VulkanHybridRenderer::PrepareComputeRaytraceUniformBuffer() 
{
	// -- Map materials uniform
	VkDeviceSize unifSize = sizeof(MaterialPacked) * m_scene->materialPackeds.size();
	m_raytrace.buffers.materials.Create(
		m_vulkanDevice,
		unifSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_vulkanDevice->MapMemory(
		m_scene->materialPackeds.data(),
		m_raytrace.buffers.materials.memory,
		unifSize
	);

	// -- Map UBO
	unifSize = sizeof(m_raytrace.ubo);
	m_raytrace.buffers.uniform.Create(
		m_vulkanDevice,
		unifSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_vulkanDevice->MapMemory(
		&m_raytrace.ubo,
		m_raytrace.buffers.uniform.memory,
		unifSize
	);
}

VkResult
VulkanHybridRenderer::PrepareComputeRaytraceTextures() {
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
	VulkanUtil::Make::SetDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_raytrace.storageRaytraceImage
	);

	return VK_SUCCESS;
}

VkResult
VulkanHybridRenderer::PrepareComputeRaytracePipeline() {

	VkComputePipelineCreateInfo computePipelineCreateInfo = MakeComputePipelineCreateInfo(m_raytrace.pipelineLayout, 0);

	VkShaderModule raytraceShader = MakeShaderModule(m_vulkanDevice->device, "shaders/hybrid/hybrid.comp.spv");
	m_logger->info("Loaded {} comp shader", "shaders/hybrid/hybrid.comp.spv");


	computePipelineCreateInfo.stage = MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, raytraceShader);

	CheckVulkanResult(
		vkCreateComputePipelines(m_vulkanDevice->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_raytrace.pipeline),
		"Failed to create compute pipeline"
	);

	VkFenceCreateInfo fenceCreateInfo = MakeFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	CheckVulkanResult(
		vkCreateFence(m_vulkanDevice->device, &fenceCreateInfo, nullptr, &m_raytrace.fence),
		"Failed to create fence"
	);

	return VK_SUCCESS;
}


VkResult
VulkanHybridRenderer::BuildComputeRaytraceCommandBuffer() {

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
	vkCmdBindDescriptorSets(m_raytrace.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_raytrace.pipelineLayout, 0, 1, &m_raytrace.descriptorSet, 0, nullptr);

	vkCmdDispatch(m_raytrace.commandBuffer, m_raytrace.storageRaytraceImage.width / 16, m_raytrace.storageRaytraceImage.height / 16, 1);

	CheckVulkanResult(
		vkEndCommandBuffer(m_raytrace.commandBuffer),
		"Failed to record command buffers"
	);

	return VK_SUCCESS;
}
