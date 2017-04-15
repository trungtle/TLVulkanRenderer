#pragma once
#include <map>
#include <vector>
#include <Typedef.h>
#include "renderer/vulkan/VulkanImage.h"

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


struct Model
{
	map<EAttrib, vector<Byte>> bytes;
	map<EAttrib, AttribInfo> attribInfo;
	VulkanImage::Image albedoMap;
	VulkanImage::Image normalMap;
};
