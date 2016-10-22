#pragma once
class Scene
{
public:
	Scene();
	~Scene();
	
	std::vector<unsigned char> m_indices;
	std::vector<unsigned char> m_positions;
};

