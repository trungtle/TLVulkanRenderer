#pragma once

#include "VulkanRenderer.h"

class Application
{
public:
	Application(
		int width = 800, 
		int height = 600
		);
	~Application();

	/**
	 * \brief This is the main loop of Application
	 */
	void Run();

private:

	int m_width;
	int m_height;
	GLFWwindow* m_window;
    VulkanRenderer* m_renderer;
};

