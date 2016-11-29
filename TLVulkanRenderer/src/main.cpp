#include <iostream>
#include "Application.h"

int main(int argc, char **argv) {
	if (argc != 2)
	{
		cout << "Usage: [gltf file]" << endl;
		return 0;
	}

    // Launch our application using the Vulkan API
	Application app(argc, argv, 800, 600, EGraphicsAPI::Vulkan);
	app.Run();
}