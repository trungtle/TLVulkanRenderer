#include "VulkanCPURayTracer.h"
#include "tinygltfloader/stb_image.h"
#include "Utilities.h"
#include "geometry/Geometry.h"
#include "scene/Camera.h"

VulkanCPURaytracer::VulkanCPURaytracer(
	GLFWwindow* window, 
	Scene* scene
	) : VulkanRenderer(window, scene), m_film{Film(m_width, m_height)}
{
	PrepareGraphics();
}

VulkanCPURaytracer::~VulkanCPURaytracer()
{
	Destroy2DImage(m_vulkanDevice, m_displayImage);

	vkDestroyImage(m_vulkanDevice->device, m_stagingImage.image, nullptr);
	vkFreeMemory(m_vulkanDevice->device, m_stagingImage.imageMemory, nullptr);
}

void 
VulkanCPURaytracer::Update() {
	
}

void 
VulkanCPURaytracer::Render() {

	VulkanRenderer::Render();

	m_film.Clear();

	VkDeviceSize imageSize = m_width * m_height * 4;

	Raytrace();

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_stagingImage.image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL
	);

	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout stagingImageLayout;
	vkGetImageSubresourceLayout(m_vulkanDevice->device, m_stagingImage.image, &subresource, &stagingImageLayout);

	// Map memory from film to screen
	void* data;
	vkMapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory, 0, imageSize, 0, &data);

	memcpy(data, m_film.GetData().data(), imageSize);

	vkUnmapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory);

	// Prepare our texture for staging
	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_stagingImage.image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_displayImage.image,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	m_vulkanDevice->CopyImage(m_graphics.queue, m_graphics.commandPool, m_displayImage.image, m_stagingImage.image, m_width, m_height);

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_displayImage.image, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}

void 
VulkanCPURaytracer::PrepareGraphics() 
{
	PrepareResources();
	PrepareGraphicsVertexBuffer();
	PrepareGraphicsDescriptorPool();
	PrepareGraphicsDescriptorSetLayout();
	PrepareGraphicsDescriptorSets();
	PrepareGraphicsPipeline();
	PrepareGraphicsCommandBuffers();

}

VkResult 
VulkanCPURaytracer::PrepareGraphicsPipeline() 
{
	VkResult result = VK_SUCCESS;

	// Load SPIR-V bytecode
	// The SPIR_V files can be compiled by running glsllangValidator.exe from the VulkanSDK or
	// by invoking the custom script shaders/compileShaders.bat
	VkShaderModule vertShader;
	PrepareShaderModule("shaders/raytracing/quad.vert.spv", vertShader);
	VkShaderModule fragShader;
	PrepareShaderModule("shaders/raytracing/quad.frag.spv", fragShader);

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

VkResult VulkanCPURaytracer::PrepareGraphicsVertexBuffer() {
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

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorPool() {
	std::array<VkDescriptorPoolSize, 1> poolSizes = {
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 1);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_graphics.descriptorPool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorSetLayout() 
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
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

	VkResult result = vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_graphics.descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
		return result;
	}

	return result;
}

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorSets() {
	VkDescriptorSetAllocateInfo allocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_graphics.descriptorSetLayout);

	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &allocInfo, &m_graphics.descriptorSets),
		"Failed to allocate descriptor set"
	);

	VkDescriptorImageInfo imageInfo = VulkanUtil::Make::MakeDescriptorImageInfo(
		VK_IMAGE_LAYOUT_GENERAL,
		m_displayImage
	);

	m_displayImage.descriptor = imageInfo;

	// Update descriptor set info
	std::array<VkWriteDescriptorSet, 1> descriptorWrites = 
	{
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_graphics.descriptorSets,
			0,
			1,
			nullptr,
			&m_displayImage.descriptor
		),
	};

	vkUpdateDescriptorSets(m_vulkanDevice->device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

	return VK_SUCCESS;
}

VkResult VulkanCPURaytracer::PrepareGraphicsCommandBuffers() {
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
			VkDeviceSize offsets[] = { 0, 0 };
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

void VulkanCPURaytracer::Raytrace() {

	// @todo make a full scene description file
	vector<Geometry*> geos;

	// Make triangles
	for (int idx = 0; idx < m_scene->indices.size(); idx++) {
		geos.push_back(
			new Triangle(
				m_scene->verticePositions[idx], 
				m_scene->verticePositions[idx + 1], 
				m_scene->verticePositions[idx + 2], 
				m_scene->verticeNormals[idx], 
				m_scene->verticeNormals[idx + 1], 
				m_scene->verticeNormals[idx + 2])
		);
	}

	vec3 light(3, 5, 10);

	for (int w = 0; w < m_width; w++) {
		for (int h = 0; h < m_height; h++) {
			Ray newRay = m_scene->camera.GenerateRay(w, h, m_width, m_height);

			for (auto geo : geos) { // Replace with acceleration structure
				Intersection isx = geo->GetIntersection(newRay);
				if (isx.t > 0)
				{
					// Shade material
					vec3 lightDirection = glm::normalize(light - isx.hitPoint);
					vec3 color = glm::clamp(glm::dot(isx.hitNormal, light), 0.1f, 1.0f) * vec3(100, 0, 0);
					m_film.SetPixel(w, h, glm::vec4(color, 1));
				}
			}
		}
	}
}

void VulkanCPURaytracer::PrepareResources() {

	VkDeviceSize imageSize = m_width * m_height * 4;

	Raytrace();

	// Stage image
	m_stagingImage = VulkanImage::Create2DImage(
		m_vulkanDevice,
		m_width,
		m_height,
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
	vkMapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory, 0, imageSize, 0, &data);

	memcpy(data, m_film.GetData().data(), imageSize);

	vkUnmapMemory(m_vulkanDevice->device, m_stagingImage.imageMemory);

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

	// Create our display image
	m_displayImage = VulkanImage::Create2DImage(
		m_vulkanDevice,
		m_width, 
		m_height,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	
	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_displayImage.image, 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_PREINITIALIZED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	m_vulkanDevice->CopyImage(m_graphics.queue, m_graphics.commandPool, m_displayImage.image, m_stagingImage.image, m_width, m_height);

	// Create image view
	m_vulkanDevice->CreateImageView(
		m_displayImage.image,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_displayImage.imageView
	);

	// Create image sampler
	CreateDefaultImageSampler(m_vulkanDevice->device, &m_displayImage.sampler);

	m_vulkanDevice->TransitionImageLayout(
		m_graphics.queue,
		m_graphics.commandPool,
		m_displayImage.image, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
