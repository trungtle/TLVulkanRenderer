#pragma once

#include <glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "scene/Scene.h"

using namespace std;

typedef enum {
	Vulkan,
	OpenGL // @note: not in roadmap
	,
	DirectX // @note: not in roadmap
} EGraphicsAPI;

enum ERenderingMode {
	FORWARD,
	DEFERRED,
	RAYTRACING_CPU,
	RAYTRACING_GPU,
	DEFFERRED_RAYTRACING
};

/**
 * \brief This is the base rendering interface class
 */
class Renderer {
public:
	Renderer(
		GLFWwindow* window,
		Scene* scene,
		std::shared_ptr<std::map<string, string>> config
	) : m_window(window), m_scene(scene), m_config(config) {
	};

	virtual ~Renderer() {
	};
	virtual void Update() = 0;
	virtual void Render() = 0;

protected:
	/**
	* \brief The window handle from glfw
	*/
	GLFWwindow* m_window;

	/**
	 * \brief Handle to scene
	 */
	Scene* m_scene;

	std::shared_ptr<std::map<string, string>> m_config;

	uint32_t m_width;
	uint32_t m_height;
};
