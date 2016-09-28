#pragma once

#include "VulkanRenderer.h"

class Viewer
{
public:
	Viewer(
		int width = 800, 
		int height = 600
		);
	~Viewer();

	/**
	 * \brief This is the main loop of Viewer
	 */
	void Run();

private:

	int m_width;
	int m_height;
	GLFWwindow* m_window;
};

