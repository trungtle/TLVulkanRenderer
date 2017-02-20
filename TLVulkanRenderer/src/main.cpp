#include <iostream>
#include "Application.h"

int main(int argc, char** argv) {

	// Default scenefile
	std::string sceneFile = "scenes/gltfs/duck/duck.gltf";
	if (argc != 2) {
		cout << "Missing scene file input! Loading default scene..." << endl;
	} else {
		sceneFile = argv[1];
	}

	// Launch our application using the Vulkan API
	Application::PreInitialize(sceneFile, 800, 600, EGraphicsAPI::Vulkan, ERenderingMode::RAYTRACING_CPU);
	Application::GetInstanced()->Run();
	Application::Destroy();
}
