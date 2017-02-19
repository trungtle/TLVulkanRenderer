#pragma once

class Scene;

class SceneLoader
{
public:
	virtual bool Load(std::string fileName, Scene* scene) = 0;
};