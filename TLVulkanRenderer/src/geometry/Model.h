#pragma once
#include <map>
#include <vector>
#include <Typedef.h>
#include "renderer/vulkan/VulkanImage.h"
#include <memory>

using namespace std;

typedef enum
{
	INDEX,
	POSITION,
	NORMAL,
	TEXCOORD,
	COLOR,
	TANGENT,
	BITANGENT,
	MATERIALID,
	WIREFRAME,
	POLYGON
} EAttrib;

typedef struct 
{
	size_t	byteStride;
	size_t	count;
	int		componentLength;
	int		componentTypeByteSize;
} AttribInfo;

enum EMap {
	ALBEDO_MAP,
	NORMAL_MAP,
	ROUGHNESS_MAP,
	METALLIC_MAP,
	AO_MAP,
	EMISSIVE_MAP
};


class Model
{
public:

	Model() : descriptorSet(nullptr) 
	{};


	~Model() {
		for (auto t : textures) {
			delete t.second;
		}
	}

	map<EAttrib, vector<Byte>> bytes;
	map<EAttrib, AttribInfo> attribInfo;
	map<EMap, VulkanImage::Image> maps;
	map<EMap, Texture*> textures;

	VkDescriptorSet descriptorSet;

};
