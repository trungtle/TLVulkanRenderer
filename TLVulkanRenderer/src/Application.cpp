#include <iostream>
#include <functional>

#include "Application.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "renderer/vulkan/VulkanGPURaytracer.h"
#include "scene/Camera.h"
#include "renderer/vulkan/VulkanCPURayTracer.h"

static int fpstracker;
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

float zoom = 0, theta = 0, phi = 0, translateX = 0, translateY = 0;
int width = 800;
int height = 600;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
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
	if (leftMousePressed) {
		// compute new camera parameters
		phi = (xpos - lastX) * 10.f / width;
		theta = (ypos - lastY) * 10.f / height;
		camchanged = true;
	}
	else if (rightMousePressed) {
		zoom += (ypos - lastY) * 10.0f / height;
		camchanged = true;
	}
	else if (middleMousePressed) {
		translateX = (xpos - lastX) * 10.f / width;
		translateY = (ypos - lastY) * 10.f / height;
		camchanged = true;
	}
	lastX = xpos;
	lastY = ypos;
}

Application* Application::pApp = nullptr;
string Application::sceneFile = "";
int Application::width = 0;
int Application::height = 0;
EGraphicsAPI Application::useAPI = EGraphicsAPI::Vulkan;
ERenderingMode Application::renderingMode = ERenderingMode::FORWARD;

void Application::PreInitialize(
	std::string sceneFile,
	int width,
	int height,
	EGraphicsAPI useAPI,
	ERenderingMode renderindMode
) {
	Application::sceneFile = sceneFile;
	Application::width = width;
	Application::height = height;
	Application::useAPI = useAPI;
	Application::renderingMode = renderindMode;
};

Application* Application::GetInstanced() {
	if (Application::pApp == nullptr)
	{
		Application::pApp = new Application(
			Application::sceneFile,
			Application::width,
			Application::height,
			Application::useAPI,
			Application::renderingMode);
	}

	return Application::pApp;
}


Application::Application(
	std::string sceneFile,
	int width,
	int height,
	EGraphicsAPI useAPI,
	ERenderingMode renderingMode
) :
	m_window(nullptr) {
	// Initialize glfw
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(width, height, "Vulkan renderer", nullptr, nullptr);

	if (!m_window) {
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(m_window);
	glfwSetKeyCallback(m_window, keyCallback);
	glfwSetCursorPosCallback(m_window, mousePositionCallback);
	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);

	std::map<std::string, std::string> config = {
		{ "USE_SBVH", "false" },
		{ "VISUALIZE_SBVH", "false"}
	};
	m_scene = new Scene(sceneFile, config);

	std::shared_ptr<map<string, string>> configPtr(&config);

	switch (useAPI) {
	case EGraphicsAPI::Vulkan:
		// Init Vulkan

		switch (renderingMode) {
		case ERenderingMode::RAYTRACING_GPU:
			m_renderer = new VulkanGPURaytracer(m_window, m_scene, configPtr);
			break;
		case ERenderingMode::RAYTRACING_CPU:
			m_renderer = new VulkanCPURaytracer(m_window, m_scene, configPtr);
			break;
		default:
			m_renderer = new VulkanRenderer(m_window, m_scene, configPtr);
			break;
		}
		break;
	default:
		std::cout << "Graphics API not supported\n";
		break;
	}

	fpstracker = 0;

}


Application::~Application() {
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
		static auto previousFrame = start;
		auto now = std::chrono::system_clock::now();
		float timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		if (timeElapsed >= 1000) {
			fps = fpstracker / (timeElapsed / 1000);
			fpstracker = 0;
			start = now;
		}

		// Update title bar
		string title = "TL Vulkan Renderer | " + std::to_string(fps) + " FPS | " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now - previousFrame).count()) + " ms / frame";
		previousFrame = now;

		glfwSetWindowTitle(m_window, title.c_str());

		// Update camera
		if (camchanged) {
			m_scene->camera.RotateAboutRight(phi);
			m_scene->camera.RotateAboutUp(theta);
			m_scene->camera.TranslateAlongRight(-translateX);
			m_scene->camera.TranslateAlongUp(translateY);
			m_scene->camera.Zoom(zoom);
			zoom = 0;
			theta = 0;
			phi = 0;
			translateX = 0;
			translateY = 0;
			camchanged = false;
		}

		// Draw
		m_renderer->Update();
		m_renderer->Render();

		fpstracker++;
	}
}
