#include <iostream>
#include <functional>

#include "Application.h"

Application::Application(
	int width,
	int height,
    EGraphicsAPI useAPI
	) : 
	m_width(width), 
	m_height(height), 
    m_useGraphicsAPI(useAPI),
    m_window(nullptr)
{
	// Initialize glfw
	glfwInit();
	
	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(m_width, m_height, "Trung's Vulkan rasterizer", nullptr, nullptr);

    switch(m_useGraphicsAPI) 
    {
        case EGraphicsAPI::Vulkan:
            // Init Vulkan
            m_renderer = new VulkanRenderer(m_window);
            break;
        default:
            std::cout << "Graphics API not supported\n";
            break;
    }

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
		m_renderer->Render();
	}
}
