#pragma once

#include "renderer/Renderer.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "scene/Scene.h"

class Application {
public:

	static string sceneFile;
	static int width;
	static int height;
	static EGraphicsAPI useAPI;
	static ERenderingMode renderingMode;

	static void PreInitialize(
		std::string sceneFile,
		int width = 800,
		int height = 600,
		EGraphicsAPI useAPI = EGraphicsAPI::Vulkan, /* Default with Vulkan */
		ERenderingMode renderindMode = ERenderingMode::RAYTRACING_CPU
	);

	static Application* GetInstanced();
	/**
	 * \brief This is the main loop of Application
	 */

	void Run();

	static void Destroy() {
		delete Application::pApp;
	};

private:

	Application(
		std::string sceneFile,
		int width,
		int height,
		EGraphicsAPI useAPI,
		ERenderingMode renderindMode
	);
	~Application();

	static Application* pApp;

	GLFWwindow* m_window;
	Scene* m_scene;
	Renderer* m_renderer;

};
