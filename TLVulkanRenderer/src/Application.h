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

	static Application* GetInstance();

	static Scene* GetScene();
	/**
	 * \brief This is the main loop of Application
	 */

	void Run();
	void ResetCamera();
	void ToggleBVHDebug();
	void ToggleSSAO();

	static void Destroy() {
		delete Application::pApp;
	};

	Scene* pScene;

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
	Renderer* m_renderer;
	std::map<std::string, std::string> m_config;

};
