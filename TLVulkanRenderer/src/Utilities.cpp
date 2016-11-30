#include <fstream>
#include <vector>

#include "Utilities.h"

glm::vec4 
NormalizeColor(int r, int g, int b, int alpha) 
{
	return glm::vec4(r, g, b, alpha) / 255.0f;
}

std::vector<Byte> 
ReadBinaryFile(
	const std::string& fileName	
	) 
{
	// Read from the end (ate flag) to determine the file size
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<Byte> buffer(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	return buffer;
}

void
LoadSPIR_V(
	const char* filePath, 
	std::vector<Byte>& outShader
	)
{
	outShader = ReadBinaryFile(filePath);
}