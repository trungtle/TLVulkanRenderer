#pragma once
#include "Geometry.h"

class Cube : public Geometry
{
public:

	Cube(vec3 position, vec3 scale, Material* material = nullptr)
	{
		m_transform = Transform(position, vec3(0), scale);
		m_material = material;
		m_area = 2.f * (scale.x * scale.y + scale.y * scale.z + scale.x * scale.z);
	}

	glm::vec4 GetCubeNormal(const glm::vec4& P) const {
		int idx = 0;
		float val = -1;
		for (int i = 0; i < 3; i++)
		{
			if (glm::abs(P[i]) > val)
			{
				idx = i;
				val = glm::abs(P[i]);
			}
		}
		glm::vec4 N(0, 0, 0, 0);
		N[idx] = glm::sign(P[idx]);
		return N;
	}

	glm::vec4 GetCubeTangent(const glm::vec4& P) const {
		int idx = 0;
		float val = -1;
		for (int i = 0; i < 3; i++)
		{
			if (glm::abs(P[i]) > val)
			{
				idx = i;
				val = glm::abs(P[i]);
			}
		}
		glm::vec4 T(0, 0, 0, 0);
		T[idx] = glm::sign(P[(idx + 1) % 3]);
		return T;
	}

	Intersection GetIntersection(const Ray& r) override;

	UV GetUV(const vec3& point) const override {
		glm::vec3 abs = glm::min(glm::abs(point), 0.5f);
		glm::vec2 UV;//Always offset lower-left corner
		if (abs.x > abs.y && abs.x > abs.z)
		{
			UV = glm::vec2(point.z + 0.5f, point.y + 0.5f) / 3.0f;
			//Left face
			if (point.x < 0)
			{
				UV += glm::vec2(0, 0.333f);
			}
			else
			{
				UV += glm::vec2(0, 0.667f);
			}
		}
		else if (abs.y > abs.x && abs.y > abs.z)
		{
			UV = glm::vec2(point.x + 0.5f, point.z + 0.5f) / 3.0f;
			//Left face
			if (point.y < 0)
			{
				UV += glm::vec2(0.333f, 0.333f);
			}
			else
			{
				UV += glm::vec2(0.333f, 0.667f);
			}
		}
		else
		{
			UV = glm::vec2(point.x + 0.5f, point.y + 0.5f) / 3.0f;
			//Left face
			if (point.z < 0)
			{
				UV += glm::vec2(0.667f, 0.333f);
			}
			else
			{
				UV += glm::vec2(0.667f, 0.667f);
			}
		}
		return UV;
	}

	BBox GetBBox() override;

	void GenerateWireframeVertices(
		const int offset,
		std::vector<uint16_t>& indices, 
		std::vector<SWireframeVertexLayout>& vertices,
		const ColorRGB& color
	);
};
