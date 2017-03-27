#pragma once
#include <vector>
#include <Typedef.h>

class Texture {
public:
	virtual glm::vec3 value(glm::vec2 uv, const glm::vec3& p) const = 0;
	virtual bool hasRawByte() const {
		return false;
	}
	virtual const Byte* getRawByte() const {
		return nullptr;
	};

	virtual uint32_t width() {
		return 0;
	}

	virtual uint32_t height() {
		return 0;
	}

	virtual ~Texture() {};
};

class CheckerTexture : public Texture {
public:
	CheckerTexture() {};
	glm::vec3 value(glm::vec2 uv, const glm::vec3& p) const override {
		float sines = sin(10 * p.x) * sin(10 * p.y) * sin(10 * p.z);
		if (sines < 0) {
			return glm::vec3(0.2, 0.3, 0.1);
		} else {
			return glm::vec3(0.9, 0.9, 0.9);
		}
	}
};

class ImageTexture : public Texture {
public:
	ImageTexture(std::string name, std::vector<unsigned char> data, Byte* rawBytes, int width, int height) :
		m_name(name), m_image(data), m_bytes(rawBytes), m_width(width), m_height(height)
	{};

	glm::vec3 value(glm::vec2 uv, const glm::vec3& p) const override {
		int i = uv.s * m_width;
		int j = (uv.t) * m_height - 0.001;
		if (i < 0) i = 0;
		if (j < 0) j = 0;
		if (i > m_width - 1) i = m_width - 1;
		if (j > m_height - 1) j = m_height - 1;

		int index = 3 * i + 3 * m_width * j;
		float r = int(m_image[index]) / 255.0;
		float g = int(m_image[index + 1]) / 255.0;
		float b = int(m_image[index + 2]) / 255.0;

		return glm::vec3(r, g, b);
	}

	bool hasRawByte() const final {
		return true;
	}

	const Byte* getRawByte() const final {
		return m_bytes;
	}

	uint32_t height() final {
		return m_height;
	}

	uint32_t width() final {
		return m_width;
	}	

private:
	std::string m_name;
	uint32_t m_width;
	uint32_t m_height;
	std::vector<unsigned char> m_image;
	Byte* m_bytes;

};
