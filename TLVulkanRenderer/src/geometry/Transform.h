#pragma once
#include <glm/glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <MathUtil.h>

class Transform
{
public:
	Transform() : translation{ glm::vec3(0) }, rotation{ glm::vec3(0) }, scale{ glm::vec3(1) } {};
	Transform(const glm::vec3 &t, const glm::vec3 &r, const glm::vec3 &s) :
		translation(t), rotation(r), scale(s) {
		SetMatrices();
	};

	void SetMatrices() {
		worldTransform = glm::translate(glm::mat4(1.0f), translation)
			* glm::rotate(glm::mat4(1.0f), rotation.x * DEG2RAD, glm::vec3(1, 0, 0))
			* glm::rotate(glm::mat4(1.0f), rotation.y * DEG2RAD, glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4(1.0f), rotation.z * DEG2RAD, glm::vec3(0, 0, 1))
			* glm::scale(glm::mat4(1.0f), scale);
		inverse_worldTransform = glm::inverse(worldTransform);
		inverse_transpose_worldTransform = glm::inverse(glm::transpose(worldTransform));
	};

	const glm::mat4 &T() const {
		return worldTransform;
	};
	const glm::mat4 &invT() const {
		return inverse_worldTransform;
	};
	const glm::mat4 &invTransT() const {
		return inverse_transpose_worldTransform;
	};

	const glm::vec3 &position() const {
		return translation;
	};

private:
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::mat4 worldTransform;
	glm::mat4 inverse_worldTransform;
	glm::mat4 inverse_transpose_worldTransform;
};