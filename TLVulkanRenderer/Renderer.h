#pragma once

#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Scene.h"

using namespace std;

typedef enum
{
    Vulkan
    , OpenGL // @note: not in roadmap
    , DirectX // @note: not in roadmap
} EGraphicsAPI;


/**
 * \brief This is the base rendering interface class
 */
class Renderer
{
public:
    Renderer(
        GLFWwindow* window,
		Scene* scene
    ) : m_window(window), m_scene(scene) {};
    virtual ~Renderer() {};
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
};

