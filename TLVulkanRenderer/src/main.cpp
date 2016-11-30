#include <iostream>
#include "Application.h"

int main(int argc, char** argv) {

	// Default scenefile
	std::string sceneFile = "../../TLVulkanRenderer/scenes/gltfs/duck/duck.gltf";
	if (argc != 2) {
		cout << "Missing scene file input! Loading default scene..." << endl;
	} else {
		sceneFile = argv[1];
	}

	// Launch our application using the Vulkan API
	Application app(sceneFile, 800, 600, EGraphicsAPI::Vulkan);
	app.Run();
}
