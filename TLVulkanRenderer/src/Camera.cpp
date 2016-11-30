#include "Camera.h"

Camera::Camera() :
	Camera(800, 600) {
}

Camera::Camera(
	float width,
	float height
) : resolution(glm::ivec2(width, height)),
    fov(45),
    position(glm::vec3(0.0f, 2.5f, 5.0f)),
    lookAt(glm::vec3(0.0f, 2.5f, 0.0f)),
    up(glm::vec3(0.0f, 1.0f, 0.0f)),
    right(glm::vec3(1.0f, 0.0f, 0.0f)),
    nearClip(0.001f),
    farClip(1000.0f),
    isPerspective(true) {
	forward = lookAt - position;
	RecomputeAttributes();
}

void
Camera::EnablePerspective(
	bool enabled
) {
	isPerspective = enabled;

	if (!isPerspective) {
		position = glm::vec3(0.0, 0.0, 10.0);
	}
	else {
		position = glm::vec3(10.0f, 10.0f, -20.0f);
	}
	RecomputeAttributes();
}

void
Camera::RecomputeAttributes() {
	forward = glm::normalize(lookAt - position);
	right = glm::normalize(glm::cross(forward, up));

	viewMat = glm::lookAt(
		position,
		lookAt,
		up
	);

	if (isPerspective) {
		projMat = glm::perspective(
			fov,
			(float)resolution.x / (float)resolution.y,
			nearClip,
			farClip
		);
	}
	else {
		projMat = glm::ortho(
			0.0f,
			(float)resolution.x,
			0.0f,
			(float)resolution.y
		);
	}
}

glm::mat4
Camera::GetViewProj() const {
	return projMat * viewMat;
}

void
Camera::RotateAboutRight(
	float deg
) {
	float rad = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rad, up);
	lookAt = lookAt - position;
	lookAt = glm::vec3(rotation * glm::vec4(lookAt, 1.0f));
	lookAt = lookAt + position;
	RecomputeAttributes();
}

void
Camera::RotateAboutUp(
	float deg
) {
	float rad = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rad, right);
	lookAt = lookAt - position;
	lookAt = glm::vec3(rotation * glm::vec4(lookAt, 1.0f));
	lookAt = lookAt + position;
	RecomputeAttributes();
}

void
Camera::Zoom(
	float amount
) {
	glm::vec3 translation = forward * amount;
	position += translation;
	lookAt += translation;
	RecomputeAttributes();
}

void
Camera::TranslateAlongRight(
	float amt
) {
	glm::vec3 translation = right * amt;
	position += translation;
	lookAt += translation;
	RecomputeAttributes();
}

void
Camera::TranslateAlongUp(
	float amt
) {
	glm::vec3 translation = up * amt;
	position += translation;
	lookAt += translation;
	RecomputeAttributes();
}
