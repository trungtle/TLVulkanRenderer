#include <iostream>
#include <functional>

#include "Application.h"

static int frame;
static int fpstracker;
static double seconds;
static int fps = 0;

Application::Application(
	int argc, 
	char **argv,
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

	// Extra filename
	std::string inputFilename(argv[1]);
	m_scene = new Scene(inputFilename);

    switch(m_useGraphicsAPI) 
    {
        case EGraphicsAPI::Vulkan:
            // Init Vulkan
            m_renderer = new VulkanRenderer(m_window, m_scene);
            break;
        default:
            std::cout << "Graphics API not supported\n";
            break;
    }

	frame = 0;
	seconds = time(NULL);
	fpstracker = 0;

}


Application::~Application()
{
	delete m_scene;
    delete m_renderer;
    glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::Run() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		static auto start = std::chrono::system_clock::now();
		auto now = std::chrono::system_clock::now();
		float timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		if (timeElapsed >= 1000)
		{
			fps = fpstracker / (timeElapsed / 1000);
			fpstracker = 0;
			start = now;
		}

		string title = "Vulkan Rasterizer | " + std::to_string(fps) + " FPS | " + std::to_string(timeElapsed) + " ms";
		glfwSetWindowTitle(m_window, title.c_str());

		m_renderer->Update();
		m_renderer->Render();

		frame++;
		fpstracker++;
	}
}
