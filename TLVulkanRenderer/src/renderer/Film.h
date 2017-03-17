#pragma once
#include <vector>
#include <glm/glm.hpp>

class Film
{
public:
	// Assume 4 channels
	Film(uint32_t width, uint32_t height) :
		m_width(width), m_height(height), m_data{std::vector<char>(width * height * 4) }
	{}

	void SetPixel(int x, int y, glm::vec4 color) {
		int offset = (x + y * m_width) * 4;
		m_data[offset] = color.r;
		m_data[offset + 1] = color.g;
		m_data[offset + 2] = color.b;
		m_data[offset + 3] = color.a;
	}

	void Clear() {
		m_data = vector<char>(m_width * m_height * 4);
	}


	std::vector<char>& GetData() {
		return m_data;
	}

	inline uint32_t GetWidth() const {
		return m_width;
	}

	inline uint32_t GetHeight() const {
		return m_height;
	}

private:
	uint32_t m_width;
	uint32_t m_height;
	std::vector<char> m_data;
};
