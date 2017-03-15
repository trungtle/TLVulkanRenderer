#pragma once
#include <vector>

class Texture {
public:
	virtual glm::vec3 value(glm::vec2 uv, const glm::vec3& p) const = 0;
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
	ImageTexture(std::string name, std::vector<unsigned char> data, int width, int height) :
		name(name), image(data), width(width), height(height)
	{};

	glm::vec3 value(glm::vec2 uv, const glm::vec3& p) const override {
		int i = uv.s * width;
		int j = (uv.t) * height - 0.001;
		if (i < 0) i = 0;
		if (j < 0) j = 0;
		if (i > width - 1) i = width - 1;
		if (j > height - 1) j = height - 1;

		int index = 3 * i + 3 * width * j;
		float r = int(image[index]) / 255.0;
		float g = int(image[index + 1]) / 255.0;
		float b = int(image[index + 2]) / 255.0;

		return glm::vec3(r, g, b);
	}
private:
	std::string name;
	int width;
	int height;
	std::vector<unsigned char> image;

};
