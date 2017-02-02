#include "bbox.h"
#include <glm/gtc/matrix_inverse.hpp>


bool BBox::DoesIntersect(const Ray& r) const 
{
	//Transform the ray
	Ray r_loc = r.GetTransformedCopy(m_transform.invT());

	// If ray origin is inside cube, return true
	bool isInside = true;
	for (int i = 0; i < 3; i++) {
		isInside &= (r_loc.m_origin[i] < 0.5f && r_loc.m_origin[i] > -0.5f);
	}

	if (isInside) return true;

	float t_n = -1000000;
	float t_f = 1000000;
	for (int i = 0; i < 3; i++)
	{
		//Ray parallel to slab check
		if (r_loc.m_direction[i] == 0)
		{
			if (r_loc.m_origin[i] < -0.5f || r_loc.m_origin[i] > 0.5f)
			{
				return false;
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
		return true;
	}

	return false;
}

glm::vec3 BBox::Centroid(
	const glm::vec3& a,
	const glm::vec3& b
)
{
	return glm::vec3(
		(a.x + b.x) / 2.0f,
		(a.y + b.y) / 2.0f,
		(a.z + b.z) / 2.0f
	);
}

BBox BBox::BBoxUnion(const BBox& a, const BBox& b) {
	BBox ret;
	ret.max.x = glm::max(a.max.x, b.max.x);
	ret.max.y = glm::max(a.max.y, b.max.y);
	ret.max.z = glm::max(a.max.z, b.max.z);
	ret.min.x = glm::min(a.min.x, b.min.x);
	ret.min.y = glm::min(a.min.y, b.min.y);
	ret.min.z = glm::min(a.min.z, b.min.z);
	ret.centroid = BBox::Centroid(ret.max, ret.min);
	ret.m_transform = Transform(ret.centroid, glm::vec3(0), ret.max - ret.min);
	return ret;
}

BBox::EAxis BBox::BBoxMaximumExtent(const BBox& bbox) {
	glm::vec3 diag = bbox.max - bbox.min;
	if (diag.x > diag.y && diag.x > diag.z)
	{
		return EAxis::X;
	}
	else if (diag.y > diag.z)
	{
		return EAxis::Y;
	}
	else
	{
		return EAxis::Z;
	}
}
