#pragma once

#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <string>
#include <set>

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
        GLFWwindow* window
    ) : m_window(window) {};
    virtual ~Renderer() {};
	virtual void Update() = 0;
    virtual void Render() = 0;

protected:
    /**
    * \brief The window handle from glfw
    */
    GLFWwindow* m_window;
};

