#include <iostream>
#include <functional>

#include "Viewer.h"

Viewer::Viewer(
	int width,
	int height
	) : 
	m_width(width), 
	m_height(height), 
	m_window(nullptr)
{
	// Initialize glfw
	glfwInit();
	
	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(m_width, m_height, "Vulkan window", nullptr, nullptr);

	// Init Vulkan
	VulkanRenderer renderer(m_window);

}


Viewer::~Viewer()
{
	glfwDestroyWindow(m_window);

	glfwTerminate();
}

void Viewer::Run() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}
