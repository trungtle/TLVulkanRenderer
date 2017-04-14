#pragma once

#include <geometry/Ray.h>
#include <geometry/Transform.h>
#include <MathUtil.h>
#include <vector>
#include <geometry/materials/LambertMaterial.h>

using namespace glm;

class BBox;
class Geometry;

struct SWireframeVertexLayout
{
	glm::vec3 position;
	glm::vec3 color;
};

struct SPolygonVertexLayout
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

class Intersection {
public:
	Point3 hitPoint;
	Normal hitNormal;
	Normal hitTangent;
	Normal hitBitangent;
	ColorRGB hitTextureColor;
	float t;
	Geometry* hitObject;
	Intersection() : t(-1), hitObject(nullptr) {};
};

class Geometry
{
public:
	Geometry();
	virtual ~Geometry();

	virtual Intersection GetIntersection(const Ray& r) = 0;
	virtual UV GetUV(const vec3&) const = 0;
	virtual BBox GetBBox() = 0;

	virtual void SetTransform(const Transform& xform) {
		m_transform = xform;
	}

	const Transform& GetTransform() const {
		return m_transform;
	}

	Material* GetMaterial() const {
		return m_material;
	}

	void SetName(std::string& name) {
		m_name = name;
	}

	std::string GetName() const {
		return m_name;
	}

	float GetArea() const {
		return m_area;
	}

protected:
	Transform m_transform;
	Material* m_material;
	std::string m_name;
	float m_area;
};

class SquarePlane : public Geometry {
public:
	SquarePlane(glm::vec3 center, glm::vec3 scale, glm::vec3 normal, Material* material) :
	m_center(center), m_scale(scale), m_normal(normal) {
		m_transform = Transform(center, glm::vec3(0), scale);
		m_material = material;
		m_area = scale.x * scale.y;
	}

	glm::vec3 m_center;
	glm::vec3 m_scale;
	glm::vec3 m_normal;

	Intersection GetIntersection(const Ray& r) override;

	UV GetUV(const vec3& point) const override {
		return vec2(point.x + 0.5f, point.y + 0.5f);
	}

	BBox GetBBox() override;

};

struct SpherePacked
{
	glm::vec3 position;
	float radius;
};

class Sphere : public Geometry
{
public:

	Sphere() :
		Sphere(glm::vec3(0), 1, nullptr)
	{};

	Sphere(glm::vec3 center, float radius, Material* material)
	{
		m_transform = Transform(center, glm::vec3(0), glm::vec3(radius));
		m_material = material;
		m_area = 4.f * PI * radius * radius;
	}

	virtual ~Sphere() {};

	Intersection GetIntersection(const Ray& r) override;

	UV GetUV(const vec3& point) const override {
		glm::vec3 p = glm::normalize(point);
		float phi = atan2f(p.z, p.x);//glm::atan(p.x/p.z);
		if (phi < 0)
		{
			phi += TWO_PI;
		}
		float theta = glm::acos(p.y);
		return glm::vec2(1 - phi / TWO_PI, 1 - theta / PI);
	}

	BBox GetBBox() override;

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
		Material* material
	) :
		vert0(v0), vert1(v1), vert2(v2),
		norm0(n0), norm1(n1), norm2(n2)
	{
		m_material = material;
		m_area = Area(vert0, vert1, vert2);
	}

	Triangle(
		vec3 v0, vec3 v1, vec3 v2,
		vec3 n0, vec3 n1, vec3 n2,
		vec2 uv0, vec2 uv1, vec2 uv2,
		Material* material
		) :
		vert0(v0), vert1(v1), vert2(v2), 
		norm0(n0), norm1(n1), norm2(n2),
		uv0(uv0), uv1(uv1), uv2(uv2) 
	{
		m_material = material;
		m_area = Area(vert0, vert1, vert2);
	}

	Intersection GetIntersection(const Ray& r) override;

	static float Area(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3)
	{
		return glm::length(glm::cross(p1 - p2, p3 - p2)) * 0.5f;
	}

	UV GetUV(const vec3& point) const override {
		float A = m_area;
		float A0 = Area(vert1, vert2, point);
		float A1 = Area(vert0, vert2, point);
		float A2 = Area(vert0, vert1, point);
		return uv0 * (A0 / A) + uv1 * (A1 / A) + uv2 * (A2 / A);
	}

	BBox GetBBox() override;

	void SetTransform(const Transform& xform) {
		m_transform = xform;
		vert0 = xform.T() * vec4(vert0, 1);
		vert1 = xform.T() * vec4(vert1, 1);
		vert2 = xform.T() * vec4(vert2, 1);
	}
};

class Mesh : public Geometry {
public:

	std::vector<Triangle> triangles;
	Intersection GetIntersection(const Ray& r) override;

	vec2 GetUV(const vec3& point) const override {
		return vec2();
	}

	BBox GetBBox() override;

	void SetTransform(const Transform& xform) {
		Geometry::SetTransform(xform);
		for (auto& tri : triangles) {
			tri.SetTransform(xform);
		}
	}
};