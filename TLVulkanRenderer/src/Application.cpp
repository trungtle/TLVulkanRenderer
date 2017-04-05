#include <iostream>
#include <functional>
#include "glm/gtx/string_cast.hpp"
#include "Application.h"
#include "scene/Camera.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "renderer/vulkan/VulkanGPURaytracer.h"
#include "renderer/vulkan/VulkanCPURayTracer.h"
#include "renderer/vulkan/VulkanHybridRenderer.h"

static int g_fpstracker;
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
Point3		g_initialEye;
Direction	g_initialLookAt;

// ===== KEY INPUT ===== //
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key)
		{
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, GL_TRUE);
				break;
			case GLFW_KEY_W:
				Application::GetScene()->camera.Zoom(10);
				break;
			case GLFW_KEY_A:
				Application::GetScene()->camera.TranslateAlongRight(-10);
				break;
			case GLFW_KEY_S:
				Application::GetScene()->camera.Zoom(-10);
				break;
			case GLFW_KEY_D:
				Application::GetScene()->camera.TranslateAlongRight(10);
				break;
			case GLFW_KEY_Q:
				Application::GetScene()->camera.TranslateAlongUp(-10);
				break;
			case GLFW_KEY_E:
				Application::GetScene()->camera.TranslateAlongUp(10);
				break;
			case GLFW_KEY_R:
				Application::GetInstance()->ResetCamera();
				break;
			case GLFW_KEY_B:
				Application::GetInstance()->ToggleBVHDebug();
				break;
			case GLFW_KEY_N:
				Application::GetInstance()->ToggleSSAO();
				break;
		}
	}
}

// ===== MOUSE INPUT ===== //
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
ERenderingMode Application::renderingMode = ERenderingMode::RAYTRACING_CPU;

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

Application* Application::GetInstance() {
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

Scene* Application::GetScene()
{
	return GetInstance()->pScene;
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

	m_config = {
		{ "USE_SBVH", "true" },
		{ "VISUALIZE_SBVH", "false"},
		{ "VISUALIZE_RAY_COST", "true"},
		{ "SSAO", "false"}
	};
	pScene = new Scene(sceneFile, m_config);
	g_initialEye = pScene->camera.eye;
	g_initialLookAt = pScene->camera.lookAt;

	std::shared_ptr<map<string, string>> configPtr(&m_config);

	switch (useAPI) {
	case EGraphicsAPI::Vulkan:
		// Init Vulkan

		switch (renderingMode) {
		case ERenderingMode::RAYTRACING_GPU:
			m_renderer = new VulkanGPURaytracer(m_window, pScene, configPtr);
			break;
		case ERenderingMode::RAYTRACING_CPU:
			m_renderer = new VulkanCPURaytracer(m_window, pScene, configPtr);
			break;
		case ERenderingMode::HYBRID:
			m_renderer = new VulkanHybridRenderer(m_window, pScene, configPtr);
			break;
		default:
			m_renderer = new VulkanRenderer(m_window, pScene, configPtr);
			break;
		}
		break;
	default:
		std::cout << "Graphics API not supported\n";
		break;
	}

	g_fpstracker = 0;

}


Application::~Application() {
	delete pScene;
	delete m_renderer;
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::ResetCamera() {
	pScene->camera.eye = g_initialEye;
	pScene->camera.lookAt = g_initialLookAt;
	pScene->camera.RecomputeAttributes();
}

void Application::ToggleBVHDebug() {
	auto it = m_config.find("VISUALIZE_SBVH");
	if(it != m_config.end() && it->second.compare("true") == 0) {
		m_config.at("VISUALIZE_SBVH") = "false";
	} else {
		m_config.at("VISUALIZE_SBVH") = "true";
	}
	m_renderer->RebuildCommandBuffers();
}

void Application::ToggleSSAO()
{
	auto it = m_config.find("SSAO");
	if (it != m_config.end() && it->second.compare("true") == 0)
	{
		m_config.at("SSAO") = "false";
	}
	else
	{
		m_config.at("SSAO") = "true";
	}
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
			fps = g_fpstracker / (timeElapsed / 1000);
			g_fpstracker = 0;
			start = now;
		}

		// Update title bar
		string title = "TL Vulkan Renderer | " + std::to_string(fps) + " FPS | " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now - previousFrame).count()) + " ms / frame";
		previousFrame = now;

		glfwSetWindowTitle(m_window, title.c_str());

		// Update camera
		if (camchanged) {
			pScene->camera.RotateAboutRight(phi);
			pScene->camera.RotateAboutUp(theta);
			pScene->camera.TranslateAlongRight(-translateX);
			pScene->camera.TranslateAlongUp(translateY);
			pScene->camera.Zoom(zoom);
			zoom = 0;
			theta = 0;
			phi = 0;
			translateX = 0;
			translateY = 0;
			camchanged = false;
			std::cout << "Camera changed. Eye: " << glm::to_string(pScene->camera.eye) << ", Ref: " << glm::to_string(pScene->camera.lookAt) << std::endl;
		}

		// Draw
		m_renderer->Update();
		m_renderer->Render();

		g_fpstracker++;
	}
}
