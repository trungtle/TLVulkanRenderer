#pragma once

#include <geometry/Ray.h>
#include <geometry/Transform.h>
#include <MathUtil.h>
#include <vector>
#include <geometry/materials/LambertMaterial.h>

using namespace glm;

class Geometry;

class Intersection {
public:
	glm::vec3 hitPoint;
	glm::vec3 hitNormal;
	glm::vec3 hitTextureColor;
	float t;
	Geometry* objectHit;
	Intersection() : t(-1), objectHit(nullptr) {};
};

class Geometry
{
public:
	Geometry();
	~Geometry();

	virtual Intersection GetIntersection(Ray r) = 0;
	virtual vec2 GetUV(const vec3&) = 0;

	Transform m_transform;
	LambertMaterial* m_material;
};

class Sphere : public Geometry
{
public:

	Intersection GetIntersection(Ray r) override
	{
		//Transform the ray
		Ray r_loc = r.GetTransformedCopy(m_transform.invT());
		Intersection result;

		float A = pow(r_loc.m_direction[0], 2) + pow(r_loc.m_direction[1], 2) + pow(r_loc.m_direction[2], 2);
		float B = 2 * (r_loc.m_direction[0] * r_loc.m_origin[0] + r_loc.m_direction[1] * r_loc.m_origin[1] + r_loc.m_direction[2] * r_loc.m_origin[2]);
		float C = pow(r_loc.m_origin[0], 2) + pow(r_loc.m_origin[1], 2) + pow(r_loc.m_origin[2], 2) - 0.25f;//Radius is 0.5f
		float discriminant = B*B - 4 * A*C;
		//If the discriminant is negative, then there is no real root
		if (discriminant < 0)
		{
			return result;
		}
		float t = (-B - sqrt(discriminant)) / (2 * A);
		if (t < 0)
		{
			t = (-B + sqrt(discriminant)) / (2 * A);
		}
		if (t >= 0)
		{
			glm::vec4 P = glm::vec4(r_loc.m_origin + t*r_loc.m_direction, 1);
			result.hitPoint = glm::vec3(m_transform.T() * P);
			glm::vec3 normal = glm::normalize(glm::vec3(P));
			result.hitNormal = glm::normalize(glm::vec3(m_transform.invTransT() * (P - glm::vec4(0, 0, 0, 1))));
			result.hitTextureColor = m_material->m_colorDiffuse;
			result.t = glm::distance(result.hitPoint, r.m_origin);
			result.objectHit = this;
		}
		return result;
	}

	vec2 GetUV(const vec3& point) override {
		glm::vec3 p = glm::normalize(point);
		float phi = atan2f(p.z, p.x);//glm::atan(p.x/p.z);
		if (phi < 0)
		{
			phi += TWO_PI;
		}
		float theta = glm::acos(p.y);
		return glm::vec2(1 - phi / TWO_PI, 1 - theta / PI);
	}
};

class Cube : public Geometry {
public:
	
	glm::vec4 GetCubeNormal(const glm::vec4& P)
	{
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

	Intersection GetIntersection(Ray r) override
	{
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
			glm::vec4 P = glm::vec4(r_loc.m_origin + t_n*r_loc.m_direction, 1);
			result.hitPoint = glm::vec3(m_transform.T() * P);
			result.hitNormal = glm::normalize(glm::vec3(m_transform.invTransT() * GetCubeNormal(P)));
			result.objectHit = this;
			result.t = glm::distance(result.hitPoint, r.m_origin);
//			result.hitTextureColor = Material::GetImageColorInterp(GetUVCoordinates(glm::vec3(P)), material->texture);
			result.hitTextureColor = m_material->m_colorDiffuse;
			return result;
		}
		else
		{//If t_near was greater than t_far, we did not hit the cube
			return result;
		}
	}

	vec2 GetUV(const vec3& point) override {
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

};

class Triangle : public Geometry {
public:
	vec3 vert0;
	vec3 vert1;
	vec3 vert2;
	vec3 norm0;
	vec3 norm1;
	vec3 norm2;
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;

	Triangle(
		vec3 v0, vec3 v1, vec3 v2,
		vec3 n0, vec3 n1, vec3 n2,
		LambertMaterial* material
	) :
		vert0(v0), vert1(v1), vert2(v2),
		norm0(n0), norm1(n1), norm2(n2)
	{
		m_material = material;
	}

	Triangle(
		vec3 v0, vec3 v1, vec3 v2,
		vec3 n0, vec3 n1, vec3 n2,
		vec2 uv0, vec2 uv1, vec2 uv2,
		LambertMaterial* material
		) :
		vert0(v0), vert1(v1), vert2(v2), 
		norm0(n0), norm1(n1), norm2(n2),
		uv0(uv0), uv1(uv1), uv2(uv2) 
	{
		m_material = material;
	}

	Intersection GetIntersection(Ray r) override
	{
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
		if (abs(det) < EPSILON)
		{
			return Intersection();
		}
		float inv_det = 1.0 / det;
		vec3 tvec = r.m_origin - vert0;

		// u, v are the barycentric coordinates of the intersection point in the triangle
		// t is the distance between the ray's origin and the point of intersection
		float u, v;

		// Compute u
		u = dot(pvec, tvec) * inv_det;
		if (u < 0.0 || u > 1.0)
		{
			return Intersection();
		}

		// Compute v
		vec3 qvec = cross(tvec, edge1);
		v = dot(r.m_direction, qvec) * inv_det;
		if (v < 0.0 || (u + v) > 1.0)
		{
			return Intersection();
		}

		// Compute t
		t = dot(edge2, qvec) * inv_det;

		// Color
		glm::vec2 uv = uv0 * (1 - u - v) + uv1 * u + uv2 * v;

		isx.hitPoint = r.GetPointOnRay(t);
		isx.hitNormal = normalize(norm0 * (1 - u - v) + norm1 * u + norm2 * v);
		isx.t = t;
		isx.hitTextureColor = m_material->m_colorDiffuse;
		isx.objectHit = this;

		return isx;
	}

	static float Area(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3)
	{
		return glm::length(glm::cross(p1 - p2, p3 - p2)) * 0.5f;
	}

	vec2 GetUV(const vec3& point) override {
		float A = Area(vert0, vert1, vert2);
		float A0 = Area(vert1, vert2, point);
		float A1 = Area(vert0, vert2, point);
		float A2 = Area(vert0, vert1, point);
		return uv0 * (A0 / A) + uv1 * (A1 / A) + uv2 * (A2 / A);
	}
};

class Mesh : public Geometry {
public:

	std::vector<Triangle> triangles;
	Intersection GetIntersection(Ray r) override {

		float nearestT = -1.0f;
		Intersection nearestIsx;
		for (auto tri : triangles) {
			Intersection isx = tri.GetIntersection(r);
			if (nearestT < 0.0f && isx.t > 0) {
				// First time
				nearestT = isx.t;
				nearestIsx = isx;
			} else if (isx.t > 0 && isx.t < nearestT) {
				nearestT = isx.t;
				nearestIsx = isx;
			}
		}
		return nearestIsx;
	}

	vec2 GetUV(const vec3& point) override {
		return vec2();
	}
};