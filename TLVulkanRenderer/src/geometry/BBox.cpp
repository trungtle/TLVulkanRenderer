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
	if (t_n < t_f)// && t_n >= 0)
	{
		return true;
	}

	return false;
}

glm::vec3 BBox::Offset(const glm::vec3& point) const 
{
	glm::vec3 out = point - m_min;
	if (m_min.x < m_max.x) out.x /= (m_max.x - m_min.x);
	if (m_min.y < m_max.y) out.y /= (m_max.y - m_min.y);
	if (m_min.z < m_max.z) out.z /= (m_max.z - m_min.z);
	return out; 
}

float BBox::GetSurfaceArea() {

	// @todo: careful if we need to dynamically update bbox
	//if (m_area < 0 || m_isDirty) {
		vec3 scale = m_max - m_min;
		m_area = 2.0f * (scale.x * scale.y + scale.x * scale.z + scale.y * scale.z);
	//}

	return m_area;
}

BBox BBox::ClipGeometry(std::shared_ptr<Geometry> geom, EAxis dim) 
{
	BBox clippedBox;
	Triangle* tri = static_cast<Triangle*>(geom.get());
	Ray rays[3];
	vec3 edge0 = normalize(tri->vert1 - tri->vert0);
	rays[0] = Ray(tri->vert0, edge0);
	vec3 edge1 = tri->vert2 - tri->vert0;
	rays[1] = Ray(tri->vert0, edge1);
	vec3 edge2 = tri->vert2 - tri->vert1;
	rays[2] = Ray(tri->vert1, edge2);

	EPlane clippingPlane;
	BBoxFace nearFace, farFace;
	nearFace.m_point = m_min;
	nearFace.m_normal[dim] = -1;
	farFace.m_point = m_max;
	farFace.m_normal[dim] = 1;

	// Find intersection with each clipping planes
	float t;
	for (auto i = 0; i < 3; i++) {
		float denom = dot(nearFace.m_normal, rays[i].m_direction);
		if (denom > 1e-6) {
			vec3 d = nearFace.m_point - rays[i].m_origin;
			t = dot(d, nearFace.m_normal) / denom;
		}
	}
	clippedBox.m_isDirty = false;
	return clippedBox;
}

bool BBox::IsInside(const vec3& point) const {
	return (point.x < m_max.x && point.x > m_min.x &&
		point.y < m_max.y && point.y > m_min.y &&
		point.z < m_max.z && point.z > m_min.z);
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
	ret.m_max.x = glm::max(a.m_max.x, b.m_max.x);
	ret.m_max.y = glm::max(a.m_max.y, b.m_max.y);
	ret.m_max.z = glm::max(a.m_max.z, b.m_max.z);
	ret.m_min.x = glm::min(a.m_min.x, b.m_min.x);
	ret.m_min.y = glm::min(a.m_min.y, b.m_min.y);
	ret.m_min.z = glm::min(a.m_min.z, b.m_min.z);
	ret.m_centroid = BBox::Centroid(ret.m_max, ret.m_min);
	ret.m_transform = Transform(ret.m_centroid, glm::vec3(0), ret.m_max - ret.m_min);
	ret.m_isDirty = false;
	return ret;
}

BBox BBox::BBoxUnion(const BBox& a, const vec3& point) 
{
	BBox ret;
	ret.m_max.x = glm::max(a.m_max.x, point.x);
	ret.m_max.y = glm::max(a.m_max.y, point.y);
	ret.m_max.z = glm::max(a.m_max.z, point.z);
	ret.m_min.x = glm::min(a.m_min.x, point.x);
	ret.m_min.y = glm::min(a.m_min.y, point.y);
	ret.m_min.z = glm::min(a.m_min.z, point.z);
	ret.m_centroid = BBox::Centroid(ret.m_max, ret.m_min);
	ret.m_transform = Transform(ret.m_centroid, glm::vec3(0), ret.m_max - ret.m_min);
	ret.m_isDirty = false;
	return ret;
}

BBox BBox::BBoxOverlap(const BBox& a, const BBox& b) 
{
	BBox ret;
	ret.m_max.x = glm::min(glm::max(a.m_max.x, b.m_min.x), glm::max(a.m_min.x, b.m_max.x));
	ret.m_max.y = glm::min(glm::max(a.m_max.y, b.m_min.y), glm::max(a.m_min.y, b.m_max.y));
	ret.m_max.z = glm::min(glm::max(a.m_max.z, b.m_min.z), glm::max(a.m_min.z, b.m_max.z));
	ret.m_min.x = glm::max(glm::min(a.m_max.x, b.m_min.x), glm::min(a.m_min.x, b.m_max.x));
	ret.m_min.y = glm::max(glm::min(a.m_max.x, b.m_min.x), glm::min(a.m_min.x, b.m_max.x));
	ret.m_min.z = glm::max(glm::min(a.m_max.x, b.m_min.x), glm::min(a.m_min.x, b.m_max.x));
	ret.m_centroid = BBox::Centroid(ret.m_max, ret.m_min);
	ret.m_transform = Transform(ret.m_centroid, glm::vec3(0), ret.m_max - ret.m_min);
	ret.m_isDirty = false;
	return ret;
}

BBox BBox::BBoxFromPoints(std::vector<vec3>& points) 
{
	BBox ret;

	for (auto point : points) {
		ret.m_max.x = glm::max(ret.m_max.x, point.x);
		ret.m_max.y = glm::max(ret.m_max.y, point.y);
		ret.m_max.z = glm::max(ret.m_max.z, point.z);
		ret.m_min.x = glm::min(ret.m_min.x, point.x);
		ret.m_min.y = glm::min(ret.m_min.y, point.y);
		ret.m_min.z = glm::min(ret.m_min.z, point.z);
	}
	ret.m_centroid = BBox::Centroid(ret.m_max, ret.m_min);
	ret.m_transform = Transform(ret.m_centroid, glm::vec3(0), ret.m_max - ret.m_min);
	ret.m_isDirty = false;

	return ret;
}

EAxis BBox::BBoxMaximumExtent(const BBox& bbox) {
	glm::vec3 diag = bbox.m_max - bbox.m_min;
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
