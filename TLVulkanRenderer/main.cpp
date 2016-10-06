#include "Application.h"

int main() {

    // Launch our application using the Vulkan API
	Application app(800, 600, EGraphicsAPI::Vulkan);
	app.Run();
}