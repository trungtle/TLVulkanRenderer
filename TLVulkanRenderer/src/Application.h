#pragma once

#include "renderer/Renderer.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "Scene.h"

class Application {
public:
	Application(
		int argc,
		char** argv,
		int width = 800,
		int height = 600,
		EGraphicsAPI useAPI = EGraphicsAPI::Vulkan, /* Default with Vulkan */
		ERenderingMode renderindMode = ERenderingMode::FORWARD
	);
	~Application();

	/**
	 * \brief This is the main loop of Application
	 */
	void Run();

private:
	int m_width;
	int m_height;

	EGraphicsAPI m_useGraphicsAPI;
	ERenderingMode m_renderingMode;
	GLFWwindow* m_window;
	Scene* m_scene;
	Renderer* m_renderer;

};
