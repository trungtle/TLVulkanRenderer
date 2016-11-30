#include <iostream>
#include <functional>

#include "Application.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "renderer/vulkan/VulkanRaytracer.h"
#include "Camera.h"

static int frame;
static int fpstracker;
static double seconds;
static int fps = 0;

// For camera controls
static bool leftMousePressed = false;
static bool rightMousePressed = false;
static bool middleMousePressed = false;
static double lastX;
static double lastY;

static bool camchanged = true;
static float dtheta = 0, dphi = 0;
static glm::vec3 cammove;

float zoom = 0, theta = 0, phi = 0;
glm::vec3 cameraPosition;
glm::vec3 ogLookAt; // for recentering the camera
int width = 800;
int height = 600;

Camera g_camera;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, GL_TRUE);
				break;
			case GLFW_KEY_S:
				//saveImage();
				break;
			case GLFW_KEY_SPACE:
				//camchanged = true;
				//Camera &cam = renderState->camera;
				//cam.lookAt = ogLookAt;
				break;
		}
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	leftMousePressed = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
	rightMousePressed = (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS);
	middleMousePressed = (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS);
}

void mousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (xpos == lastX || ypos == lastY) return; // otherwise, clicking back into window causes re-start
	if (leftMousePressed)
	{
		// compute new camera parameters
		phi = (xpos - lastX) * 10.f / width;
		theta = (ypos - lastY) * 10.f / height;
		camchanged = true;
	}
	else if (rightMousePressed)
	{
		zoom += (ypos - lastY) * 10.0f / height;
		camchanged = true;
	}
	else if (middleMousePressed)
	{
		Camera &cam = g_camera;
		glm::vec3 forward = cam.forward;
		//forward.y = 0.0f;
		//forward = glm::normalize(forward);
		glm::vec3 right = cam.right;
		//right.y = 0.0f;
		//right = glm::normalize(right);

		cam.lookAt -= (float)(xpos - lastX) * right * 0.01f;
		cam.lookAt += (float)(ypos - lastY) * cam.up * 0.01f;
		camchanged = true;
	}
	lastX = xpos;
	lastY = ypos;
}


Application::Application(
	int argc, 
	char **argv,
	int width,
	int height,
    EGraphicsAPI useAPI,
	ERenderingMode renderingMode
	) : 
	m_width(width), 
	m_height(height), 
    m_useGraphicsAPI(useAPI),
	m_renderingMode(renderingMode),
    m_window(nullptr)
{
	// Initialize glfw
	glfwInit();
	
	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(m_width, m_height, "Vulkan renderer", nullptr, nullptr);

	if (!m_window)
	{
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(m_window);
	glfwSetKeyCallback(m_window, keyCallback);
	glfwSetCursorPosCallback(m_window, mousePositionCallback);
	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);

	// Extra filename
	std::string inputFilename(argv[1]);
	m_scene = new Scene(inputFilename);

	g_camera = Camera(width, height);

    switch(m_useGraphicsAPI) 
    {
        case EGraphicsAPI::Vulkan:
            // Init Vulkan

			if (m_renderingMode == ERenderingMode::RAYTRACING) {
				m_renderer = new VulkanRaytracer(m_window, m_scene);
			} else {
				m_renderer = new VulkanRenderer(m_window, m_scene);
			}
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

		// Keep track of fps
		static auto start = std::chrono::system_clock::now();
		auto now = std::chrono::system_clock::now();
		float timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		if (timeElapsed >= 1000)
		{
			fps = fpstracker / (timeElapsed / 1000);
			fpstracker = 0;
			start = now;
		}

		// Update title bar
		string title = "TL Vulkan Rasterizer | " + std::to_string(fps) + " FPS | " + std::to_string(timeElapsed) + " ms";
		glfwSetWindowTitle(m_window, title.c_str());

		// Update camera
		if (camchanged)
		{
			Camera &cam = g_camera;
			cam.TranslateAlongRight(phi);
			cam.TranslateAlongUp(theta);
			cam.Zoom(zoom);
			zoom = 0;
			theta = 0;
			phi = 0;
			camchanged = false;
		}

		m_scene->camera = &g_camera;

		// Draw
		m_renderer->Update();
		m_renderer->Render();

		frame++;
		fpstracker++;
	}
}
