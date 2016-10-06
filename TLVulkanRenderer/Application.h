#pragma once

#include "Renderer.h"
#include "VulkanRenderer.h"

class Application
{
public:
	Application(
		int width = 800, 
		int height = 600,
        EGraphicsAPI useAPI = EGraphicsAPI::Vulkan /* Default with Vulkan */
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
	GLFWwindow* m_window;
    Renderer* m_renderer;
};

