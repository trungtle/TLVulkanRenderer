#include "Geometry.h"
#include <geometry/BBox.h>

Geometry::Geometry() {
}


Geometry::~Geometry() {
}

Intersection SquarePlane::GetIntersection(const Ray & r)
{
	return Intersection();
}

BBox SquarePlane::GetBBox()
{
	return BBox();
}

Intersection Sphere::GetIntersection(const Ray& r) {
	//Transform the ray
	Ray r_loc = r.GetTransformedCopy(m_transform.invT());
	Intersection result;

	float A = pow(r_loc.m_direction[0], 2) + pow(r_loc.m_direction[1], 2) + pow(r_loc.m_direction[2], 2);
	float B = 2 * (r_loc.m_direction[0] * r_loc.m_origin[0] + r_loc.m_direction[1] * r_loc.m_origin[1] + r_loc.m_direction[2] * r_loc.m_origin[2]);
	float C = pow(r_loc.m_origin[0], 2) + pow(r_loc.m_origin[1], 2) + pow(r_loc.m_origin[2], 2) - 0.25f;//Radius is 0.5f
	float discriminant = B * B - 4 * A * C;
	//If the discriminant is negative, then there is no real root
	if (discriminant < 0) {
		return result;
	}
	float t = (-B - sqrt(discriminant)) / (2 * A);
	if (t < 0) {
		t = (-B + sqrt(discriminant)) / (2 * A);
	}
	if (t >= 0) {
		glm::vec4 P = glm::vec4(r_loc.m_origin + t * r_loc.m_direction, 1);
		result.hitPoint = glm::vec3(m_transform.T() * P);
		glm::vec3 normal = glm::vec3(P);
		result.hitNormal = glm::normalize(glm::vec3(m_transform.invTransT() * glm::vec4(normal, 0)));

		// To compute tangent, convert to spherical coordinate, then rotate by pi/2
		float radius = glm::length(P);
		float theta = acos(P.z / radius);
		float phi = atan2(P.y, P.x);
		theta += PI / 2.0;
		Direction tangent(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		result.hitTangent = glm::normalize(glm::vec3(m_transform.invTransT() * glm::vec4(tangent, 0 )));
		result.hitBitangent = glm::cross(result.hitNormal, result.hitTangent);

		result.hitTextureColor = m_material->m_colorDiffuse;
		result.t = glm::distance(result.hitPoint, r.m_origin);
		result.hitObject = this;
	}
	return result;
}

BBox Sphere::GetBBox() {
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


Intersection Triangle::GetIntersection(const Ray& r) {
	Intersection isx;
	// Compute fast intersection using Muller and Trumbore, this skips computing the plane's equation.
	// See https://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

	float t = -1.0;

	// Find the edges that share vertice 0
	vec3 edge1 = vert1 - vert0;
	vec3 edge2 = vert2 - vert0;

	// Being computing determinante. Store pvec for recomputation
	vec3 pvec = cross(r.m_direction, edge2);
	// If determinant is 0, ray lies in plane of triangle
	float det = dot(pvec, edge1);
	if (fabs(det) < EPSILON) {
		return Intersection();
	}
	float inv_det = 1.0f / det;
	vec3 tvec = r.m_origin - vert0;

	// u, v are the barycentric coordinates of the intersection point in the triangle
	// t is the distance between the ray's origin and the point of intersection
	float u, v;

	// Compute u
	u = dot(pvec, tvec) * inv_det;
	if (u < 0.0 || u > 1.0) {
		return Intersection();
	}

	// Compute v
	vec3 qvec = cross(tvec, edge1);
	v = dot(r.m_direction, qvec) * inv_det;
	if (v < 0.0 || (u + v) > 1.0) {
		return Intersection();
	}

	// Compute t
	t = dot(edge2, qvec) * inv_det;

	// Color
	glm::vec2 uv = uv0 * (1 - u - v) + uv1 * u + uv2 * v;

	CheckerTexture checkerTexture;
	isx.hitPoint = r.GetPointOnRay(t);
	isx.hitNormal = normalize(norm0 * (1 - u - v) + norm1 * u + norm2 * v);
	isx.hitTangent = normalize(vert0 - isx.hitPoint); // @todo: For now, pick any tangent
	isx.hitBitangent = glm::cross(isx.hitNormal, isx.hitTangent);
	isx.t = t;
	isx.hitTextureColor = m_material->m_texture == nullptr ? checkerTexture.value(uv, isx.hitPoint) : m_material->m_texture->value(uv, isx.hitPoint);
	isx.hitObject = this;

	return isx;
}

BBox Triangle::GetBBox() {
	BBox box;
	glm::vec3 p0 = glm::vec3(m_transform.T() * glm::vec4(vert0, 1.f));
	glm::vec3 p1 = glm::vec3(m_transform.T() * glm::vec4(vert1, 1.f));
	glm::vec3 p2 = glm::vec3(m_transform.T() * glm::vec4(vert2, 1.f));
	box.m_min.x = min(p0.x, min(p1.x, p2.x));
	box.m_min.y = min(p0.y, min(p1.y, p2.y));
	box.m_min.z = min(p0.z, min(p1.z, p2.z));
	box.m_max.x = max(p0.x, max(p1.x, p2.x));
	box.m_max.y = max(p0.y, max(p1.y, p2.y));
	box.m_max.z = max(p0.z, max(p1.z, p2.z));
	box.m_centroid = BBox::Centroid(box.m_min, box.m_max);
	box.m_transform = Transform(box.m_centroid, glm::vec3(0), box.m_max - box.m_min);
	box.m_isDirty = false;

	return box;
}

Intersection Mesh::GetIntersection(const Ray& r) {

	Ray r_loc = r.GetTransformedCopy(m_transform.invT());

	float nearestT = -1.0f;
	Intersection nearestIsx;
	for (auto tri : triangles) {
		Intersection isx = tri.GetIntersection(r_loc);
		if (nearestT < 0.0f && isx.t > 0) {
			// First time
			nearestT = isx.t;
			nearestIsx = isx;
		}
		else if (isx.t > 0 && isx.t < nearestT) {
			nearestT = isx.t;
			nearestIsx = isx;
		}
	}
	return nearestIsx;
}

BBox Mesh::GetBBox() {
	BBox result;
	for (auto g : triangles)
	{
		BBox box = g.GetBBox();
		result = BBox::BBoxUnion(result, box);
	}
	return result;
}
