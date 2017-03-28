#include "Cube.h"
#include "BBox.h"

Intersection Cube::GetIntersection(const Ray& r) {
	//Transform the ray
	Ray r_loc = r.GetTransformedCopy(m_transform.invT());
	Intersection result;

	float t_n = -1000000;
	float t_f = 1000000;
	for (int i = 0; i < 3; i++)
	{
		//Ray parallel to slab check
		if (r_loc.m_direction[i] == 0)
		{
			if (r_loc.m_origin[i] < -0.5f || r_loc.m_origin[i] > 0.5f)
			{
				return result;
			}
		}
		//If not parallel, do slab intersect check
		float t0 = (-0.5f - r_loc.m_origin[i]) / r_loc.m_direction[i];
		float t1 = (0.5f - r_loc.m_origin[i]) / r_loc.m_direction[i];
		if (t0 > t1)
		{
			float temp = t1;
			t1 = t0;
			t0 = temp;
		}
		if (t0 > t_n)
		{
			t_n = t0;
		}
		if (t1 < t_f)
		{
			t_f = t1;
		}
	}
	if (t_n < t_f && t_n >= 0)
	{
		//Lastly, transform the point found in object space by T
		glm::vec4 P = glm::vec4(r_loc.m_origin + t_n * r_loc.m_direction, 1);
		result.hitPoint = glm::vec3(m_transform.T() * P);
		result.hitNormal = glm::normalize(glm::vec3(m_transform.invTransT() * GetCubeNormal(P)));
		result.hitTangent = glm::normalize(glm::vec3(m_transform.invTransT() * GetCubeTangent(P)));
		result.hitBitangent = glm::cross(result.hitNormal, result.hitTangent);
		result.hitObject = this;
		result.t = glm::distance(result.hitPoint, r.m_origin);
		result.hitTextureColor = m_material->m_colorDiffuse;
		return result;
	}
	else
	{//If t_near was greater than t_far, we did not hit the cube
		return result;
	}
}

BBox Cube::GetBBox() {
	BBox bbox;

	// Convert to world first
	glm::vec3 p0 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p1 = glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, .5f, 1.0f));
	glm::vec3 p2 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, -.5f, 1.0f));
	glm::vec3 p3 = glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, .5f, 1.0f));
	glm::vec3 p4 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, -.5f, 1.0f));
	glm::vec3 p5 = glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, .5f, 1.0f));
	glm::vec3 p6 = glm::vec3(m_transform.T()* glm::vec4(.5f, .5f, -.5f, 1.0f));
	glm::vec3 p7 = glm::vec3(m_transform.T() * glm::vec4(.5f, .5f, .5f, 1.0f));

	bbox.m_min = glm::min(p0, glm::min(p1, glm::min(p2, glm::min(p3, glm::min(p4, glm::min(p5, glm::min(p6, p7)))))));
	bbox.m_max = glm::max(p0, glm::max(p1, glm::max(p2, glm::max(p3, glm::max(p4, glm::max(p5, glm::max(p6, p7)))))));
	bbox.m_centroid = BBox::Centroid(bbox.m_min, bbox.m_max);
	bbox.m_transform = Transform(bbox.m_centroid, glm::vec3(0), bbox.m_max - bbox.m_min);
	bbox.m_isDirty = false;
	return bbox;
}

void Cube::GenerateWireframeVertices(
	const int offset,
	std::vector<uint16_t>& indices, 
	std::vector<SWireframeVertexLayout>& vertices,
	const ColorRGB& color
	) 
{
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(.5f, .5f, .5f, 1)),
		color
	});
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(.5f, .5f, -.5f, 1)),
		color
	});
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, .5f, 1)),
		color
	});
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(.5f, -.5f, -.5f, 1)),
		color
	});
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, .5f, 1)),
		color
	});
	vertices.push_back(
	{
		glm::vec3(m_transform.T() * glm::vec4(-.5f, .5f, -.5f, 1)),
		color
	});
	vertices.push_back({
		glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, .5f, 1)),
		color
	});
	vertices.push_back({
		glm::vec3(m_transform.T() * glm::vec4(-.5f, -.5f, -.5f, 1)),
		color
	});

	// Setup indices

	indices.push_back(0 + offset);
	indices.push_back(1 + offset);
	indices.push_back(1 + offset);
	indices.push_back(3 + offset);
	indices.push_back(3 + offset);
	indices.push_back(2 + offset);
	indices.push_back(2 + offset);
	indices.push_back(0 + offset);
	indices.push_back(0 + offset);
	indices.push_back(4 + offset);
	indices.push_back(4 + offset);
	indices.push_back(6 + offset);
	indices.push_back(6 + offset);
	indices.push_back(2 + offset);
	indices.push_back(3 + offset);
	indices.push_back(7 + offset);
	indices.push_back(7 + offset);
	indices.push_back(6 + offset);
	indices.push_back(1 + offset);
	indices.push_back(5 + offset);
	indices.push_back(5 + offset);
	indices.push_back(4 + offset);
	indices.push_back(5 + offset);
	indices.push_back(7 + offset);
}
