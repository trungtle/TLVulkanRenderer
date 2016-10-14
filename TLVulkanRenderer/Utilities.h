#pragma once

/**
 * \brief Read a binary file and returns its bytes
 * \param fileName 
 * \return bytes from the binary file
 */
std::vector<char>
ReadBinaryFile(
	const std::string& fileName
	);


/**
 * \brief Load SPIR_V binary
 * \param vertShaderFilePath 
 * \param fragShaderFilePath 
 */
void 
LoadSPIR_V(
	const char* vertShaderFilePath
	, const char* fragShaderFilePath
	, std::vector<char>& outVertShader
	, std::vector<char>& outFragShader
);