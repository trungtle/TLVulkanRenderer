#include <iostream>
#include <functional>

#include "Application.h"

Application::Application(
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
	m_renderer = new VulkanRenderer(m_window);

}


Application::~Application()
{
    delete m_renderer;
    glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::Run() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}
