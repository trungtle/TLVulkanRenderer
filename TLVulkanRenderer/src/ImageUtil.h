#pragma once
#include <vector>

struct Texture {
	std::string name;
	int width;
	int height;
	int component;
	std::vector<unsigned char> image;
};
