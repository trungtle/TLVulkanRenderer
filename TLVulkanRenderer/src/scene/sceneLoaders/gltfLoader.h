#pragma once
#include "tinygltfloader/tiny_gltf_loader.h"
#include "SceneLoader.h"

class gltfLoader : public SceneLoader
{
public:
	bool Load(std::string fileName, Scene* scene) override;
};
