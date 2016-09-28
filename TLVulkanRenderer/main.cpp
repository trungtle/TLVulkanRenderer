
#include <iostream>
#include "Viewer.h"

void initVulkan() {
	
}

int main() {

	Viewer viewer(800, 600);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::cout << extensionCount << " extensions supported" << std::endl;

	glm::vec4 vec;
	glm::mat4 matrix;
	auto test = matrix * vec;
	
	viewer.Run();
}