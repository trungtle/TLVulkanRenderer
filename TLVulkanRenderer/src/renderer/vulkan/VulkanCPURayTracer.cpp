#include "VulkanCPURayTracer.h"
#include "tinygltfloader/stb_image.h"
#include "Utilities.h"
#include "geometry/Geometry.h"
#include "scene/Camera.h"

#define MULTITHREAD
#define USE_SBVH

vec3 ShadeMaterial(Scene* scene, Ray& newRay) {
	vec3 color = vec3(1, 1, 1);
	int depth = 1;
	for (auto light : scene->lights) {
		for (int i = 0; i < depth; i++)
		{
			Intersection isx = scene->GetIntersection(newRay);
			if (isx.t > 0)
			{
				vec3 lightDirection = glm::normalize(light->m_position - isx.hitPoint);

				// Shade material
				color *= isx.hitObject->m_material->EvaluateEnergy(isx, lightDirection, newRay.m_direction);
				color *= light->Attenuation(isx.hitPoint);

				// Shadow feeler
				Ray shadowFeeler(isx.hitPoint + 0.001f * lightDirection, lightDirection);
				if (scene->DoesIntersect(shadowFeeler))
				{
					color *= 0.1f;
				} else {
					color *= light->m_color;
				}
			}
			else
			{
				color *= 0;
				break;
			}
		}
	}
	return color;
}

void Raytrace(uint32_t x, uint32_t y, Scene* scene, Film* film) {

	vector<vec3> lights = {
		vec3(3, 5, 10)
	};

	Ray newRay = scene->camera.GenerateRay(x, y);
	vec3 color = ShadeMaterial(scene, newRay);
	color = glm::clamp(color * 255.0f, 0.f, 255.f);
	film->SetPixel(x, y, glm::vec4(color, 1));

}

void Task(
	uint32_t tileX,
	uint32_t tileY,
	Scene* scene,
	Film* film
)
{
	uint32_t width = scene->camera.resolution.x;
	uint32_t height = scene->camera.resolution.y;
	uint32_t sizeX = width / 4;
	uint32_t startX = tileX * sizeX;
	uint32_t endX = (tileX + 1) * sizeX;
	endX = std::min(endX, width);

	uint32_t sizeY = height / 4;
	uint32_t startY = tileY * sizeY;
	uint32_t endY = (tileY + 1) * sizeY;
	endY = std::min(endY, height);

	for (uint32_t x = startX; x < endX; x++)
	{
		for (uint32_t y = startY; y < endY; y++)
		{
			Raytrace(x, y, scene, film);
		}
	}
}


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
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float timeSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	// === Wireframe
	glm::mat4 vp = m_scene->camera.GetViewProj();

	uint8_t *pData;
	vkMapMemory(m_vulkanDevice->device, m_wireframeUniform.memory, 0, sizeof(vp), 0, (void **)&pData);
	memcpy(pData, &vp, sizeof(vp));
	vkUnmapMemory(m_vulkanDevice->device, m_wireframeUniform.memory);

}

void 
VulkanCPURaytracer::Render() {

	VulkanRenderer::Render();

	m_film.Clear();

	VkDeviceSize imageSize = m_width * m_height * 4;

#ifdef MULTITHREAD
	// Generate 4x4 threads
	for (int i = 0; i < 16; i++)
	{
		m_threads[i] = std::thread(Task, i / 4, i % 4, m_scene, &m_film);
	}
	for (int i = 0; i < 16; i++)
	{
		m_threads[i].join();
	}
#else
	for (int w = 0; w < m_width; w++)
	{
		for (int h = 0; h < m_height; h++)
		{
			Raytrace(w, h, m_scene, &m_film);
		}
	}
#endif

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
	PrepareGraphicsUniformBuffer();
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


	// === For BVH wireframe debug
	inputAssemblyStateCreateInfo = MakePipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	rasterizationStateCreateInfo = MakePipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_LINE, 
		VK_CULL_MODE_NONE, 
		VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkShaderModule wireframeVertShader;
	PrepareShaderModule("shaders/raytracing/wireframe.vert.spv", wireframeVertShader);
	VkShaderModule wireframeFragShader;
	PrepareShaderModule("shaders/raytracing/wireframe.frag.spv", wireframeFragShader);

	shaderCreateInfos = {
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, wireframeVertShader),
		MakePipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, wireframeFragShader)
	};


	attribDesc = {
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
			offsetof(SWireframe, color) // offset
		)
	};

	bindingDesc = {
		MakeVertexInputBindingDescription(
			0, // binding
			sizeof(SWireframe), // stride
			VK_VERTEX_INPUT_RATE_VERTEX
		)
	};

	VkPipelineVertexInputStateCreateInfo wireframeInputState = MakePipelineVertexInputStateCreateInfo(
		bindingDesc,
		attribDesc
	);


	graphicsPipelineCreateInfo.pVertexInputState = &wireframeInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	
	VkGraphicsPipelineCreateInfo wireframePipelineCreateInfo = 
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
		m_wireframePipelineLayout,
		m_graphics.renderPass,
		0, // Subpass

		   // Since pipelins are expensive to create, potentially we could reuse a common parent pipeline using the base pipeline handle.									
		   // We just have one here so we don't need to specify these values.
		VK_NULL_HANDLE,
		-1
	);
	
	CheckVulkanResult(
		vkCreateGraphicsPipelines(
			m_vulkanDevice->device,
			VK_NULL_HANDLE, // Pipeline caches here
			1, // Pipeline count
			&wireframePipelineCreateInfo,
			nullptr,
			&m_wireframePipeline // Pipelines
		),
		"Failed to create graphics pipeline"
	);

	vkDestroyShaderModule(m_vulkanDevice->device, wireframeVertShader, nullptr);
	vkDestroyShaderModule(m_vulkanDevice->device, wireframeFragShader, nullptr);

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

VkResult VulkanCPURaytracer::PrepareGraphicsUniformBuffer() 
{
	VulkanRenderer::PrepareGraphicsUniformBuffer();

	m_wireframeUniform.Create(
		m_vulkanDevice,
		sizeof(glm::mat4),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

	return VK_SUCCESS;
}

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes = {
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		MakeDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = MakeDescriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 5);

	CheckVulkanResult(
		vkCreateDescriptorPool(m_vulkanDevice->device, &descriptorPoolCreateInfo, nullptr, &m_graphics.descriptorPool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorSetLayout() 
{
	// === Quad
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

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_graphics.descriptorSetLayout);
	pipelineLayoutCreateInfo.pSetLayouts = &m_graphics.descriptorSetLayout;
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_graphics.pipelineLayout),
		"Failed to create pipeline layout."
	);

	// === Wireframe set layout
	setLayoutBindings = {
		MakeDescriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT
		),
	};

	descriptorSetLayoutCreateInfo =
		MakeDescriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			setLayoutBindings.size()
		);

	result = vkCreateDescriptorSetLayout(m_vulkanDevice->device, &descriptorSetLayoutCreateInfo, nullptr, &m_wireframeDescriptorLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
		return result;
	}

	pipelineLayoutCreateInfo = MakePipelineLayoutCreateInfo(&m_wireframeDescriptorLayout);
	pipelineLayoutCreateInfo.pSetLayouts = &m_wireframeDescriptorLayout;
	CheckVulkanResult(
		vkCreatePipelineLayout(m_vulkanDevice->device, &pipelineLayoutCreateInfo, nullptr, &m_wireframePipelineLayout),
		"Failed to create pipeline layout."
	);

	return result;
}

VkResult VulkanCPURaytracer::PrepareGraphicsDescriptorSets() {

	// === Quad
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

	// === Wireframe
	allocInfo = allocInfo = MakeDescriptorSetAllocateInfo(m_graphics.descriptorPool, &m_wireframeDescriptorLayout);
	CheckVulkanResult(
		vkAllocateDescriptorSets(m_vulkanDevice->device, &allocInfo, &m_wireframeDescriptorSet),
		"Failed to allocate descriptor set"
	);

	descriptorWrites =
	{
		MakeWriteDescriptorSet(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_wireframeDescriptorSet,
			0,
			1,
			&m_wireframeUniform.descriptor,
			nullptr
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


		// -- Draw BVH tree
#ifdef USE_SBVH
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindPipeline(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframePipeline);
		vkCmdBindDescriptorSets(m_graphics.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframePipelineLayout, 0, 1, &m_wireframeDescriptorSet, 0, NULL);
		vkCmdBindVertexBuffers(m_graphics.commandBuffers[i], 0, 1, &m_wireframeBVHVertices.buffer, offsets);
		vkCmdBindIndexBuffer(m_graphics.commandBuffers[i], m_wireframeBVHIndices.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(m_graphics.commandBuffers[i], m_wireframeIndexCount, 1, 0, 0, 1);
#endif
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

void VulkanCPURaytracer::PrepareResources() {

	GenerateWireframeBVHNodes();

	m_logger->info("Number of threads available: {0}\n", std::thread::hardware_concurrency());

#ifdef MULTITHREAD
	// Generate 4x4 threads
	for (int i = 0; i < 16; i++) {
		m_threads[i] = std::thread(Task, i / 4, i % 4, m_scene, &m_film);
	}
	for (int i = 0; i < 16; i++)
	{
		m_threads[i].join();
	}
#else
	for (int w = 0; w < m_width; w++) {
		for (int h = 0; h < m_height; h++) {
			Raytrace(w, h, m_scene, &m_film);
		}
	}
#endif


	VkDeviceSize imageSize = m_width * m_height * 4;


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

void VulkanCPURaytracer::GenerateWireframeBVHNodes() {


	std::vector<SWireframe> vertexBuffer;
	std::vector<uint16_t> bbox_idx;

	size_t verticeCount = 0;
	vec3 color;
	for (auto node : m_scene->m_sbvh.m_nodes)
	{
		if (node->IsLeaf())
		{
			color = vec3(0, 1, 1);
		}
		else
		{
			color = vec3(1, 0, 0);
		}
		// Setup vertices
		glm::vec3 centroid = node->bbox.centroid;
		glm::vec3 translation = centroid;
		glm::vec3 scale = glm::vec3(glm::vec3(node->bbox.max) - glm::vec3(node->bbox.min));
		glm::mat4 transform = glm::translate(glm::mat4(1.0), translation) * glm::scale(glm::mat4(1.0f), scale);

		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, .5f, .5f, 1)),
			color 
		});
		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, .5f, -.5f, 1)),
			color
		});
		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, -.5f, .5f, 1)),
			color
		});
		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(.5f, -.5f, -.5f, 1)),
			color
		});
		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(-.5f, .5f, .5f, 1)),
			color
		});
		vertexBuffer.push_back(
		{
			glm::vec3(transform * glm::vec4(-.5f, .5f, -.5f, 1)),
			color
		});
		vertexBuffer.push_back({
			glm::vec3(transform * glm::vec4(-.5f, -.5f, .5f, 1)),
			color
		});
		vertexBuffer.push_back({
			glm::vec3(transform * glm::vec4(-.5f, -.5f, -.5f, 1)),
			color
		});

		// Setup indices

		bbox_idx.push_back(0 + verticeCount);
		bbox_idx.push_back(1 + verticeCount);
		bbox_idx.push_back(1 + verticeCount);
		bbox_idx.push_back(3 + verticeCount);
		bbox_idx.push_back(3 + verticeCount);
		bbox_idx.push_back(2 + verticeCount);
		bbox_idx.push_back(2 + verticeCount);
		bbox_idx.push_back(0 + verticeCount);
		bbox_idx.push_back(0 + verticeCount);
		bbox_idx.push_back(4 + verticeCount);
		bbox_idx.push_back(4 + verticeCount);
		bbox_idx.push_back(6 + verticeCount);
		bbox_idx.push_back(6 + verticeCount);
		bbox_idx.push_back(2 + verticeCount);
		bbox_idx.push_back(3 + verticeCount);
		bbox_idx.push_back(7 + verticeCount);
		bbox_idx.push_back(7 + verticeCount);
		bbox_idx.push_back(6 + verticeCount);
		bbox_idx.push_back(1 + verticeCount);
		bbox_idx.push_back(5 + verticeCount);
		bbox_idx.push_back(5 + verticeCount);
		bbox_idx.push_back(4 + verticeCount);
		bbox_idx.push_back(5 + verticeCount);
		bbox_idx.push_back(7 + verticeCount);

		verticeCount += 8;

	}

	VkDeviceSize bufferSize = vertexBuffer.size() * sizeof(SWireframe);
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		m_wireframeBVHVertices.buffer,
		m_wireframeBVHVertices.memory
	);

	m_vulkanDevice->MapMemory(
		vertexBuffer.data(),
		m_wireframeBVHVertices.memory,
		bufferSize,
		0
	);

	bufferSize = bbox_idx.size() * sizeof(uint16_t);
	m_vulkanDevice->CreateBufferAndMemory(
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		m_wireframeBVHIndices.buffer,
		m_wireframeBVHIndices.memory
	);

	m_vulkanDevice->MapMemory(
		bbox_idx.data(),
		m_wireframeBVHIndices.memory,
		bufferSize,
		0
	);

	m_wireframeIndexCount = bbox_idx.size();
}