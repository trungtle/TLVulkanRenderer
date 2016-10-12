#include <fstream>
#include <vector>

#include "Utilities.h"

std::vector<char> 
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
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void
LoadSPIR_V(
	const char* vertShaderFilePath
	, const char* fragShaderFilePath
	, std::vector<char>& outVertShader
	, std::vector<char>& outFragShader
	)
{
	outVertShader = ReadBinaryFile(vertShaderFilePath);
	outFragShader = ReadBinaryFile(fragShaderFilePath);
}