#include "Camera.h"
#include "geometry/Geometry.h"

Camera::Camera() :
	Camera(800, 600) {
}

Camera::Camera(
	float width,
	float height
) : resolution(glm::ivec2(width, height)),
    fov(45),
    eye(glm::vec3(0.0f, 0.0f, 20.0f)),
    lookAt(glm::vec3(0.0f, 0.0f, 0.0f)),
    up(glm::vec3(0.0f, 1.0f, 0.0f)),
    right(glm::vec3(1.0f, 0.0f, 0.0f)),
    nearClip(0.001f),
    farClip(1000.0f),
    isPerspective(true) {
	forward = lookAt - eye;
	RecomputeAttributes();
}

void
Camera::EnablePerspective(
	bool enabled
) {
	isPerspective = enabled;

	if (!isPerspective) {
		eye = glm::vec3(0.0, 0.0, 10.0);
	}
	else {
		eye = glm::vec3(10.0f, 10.0f, -20.0f);
	}
	RecomputeAttributes();
}

void
Camera::RecomputeAttributes() {
	forward = glm::normalize(lookAt - eye);
	right = glm::normalize(glm::cross(forward, up));

	viewMat = glm::lookAt(
		eye,
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
	lookAt = lookAt - eye;
	lookAt = glm::vec3(rotation * glm::vec4(lookAt, 1.0f));
	lookAt = lookAt + eye;
	RecomputeAttributes();
}

void
Camera::RotateAboutUp(
	float deg
) {
	float rad = glm::radians(deg);
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rad, right);
	lookAt = lookAt - eye;
	lookAt = glm::vec3(rotation * glm::vec4(lookAt, 1.0f));
	lookAt = lookAt + eye;
	RecomputeAttributes();
}

void
Camera::Zoom(
	float amount
) {
	glm::vec3 translation = forward * amount;
	eye += translation;
	lookAt += translation;
	RecomputeAttributes();
}

void
Camera::TranslateAlongRight(
	float amt
) {
	glm::vec3 translation = right * amt;
	eye += translation;
	lookAt += translation;
	RecomputeAttributes();
}

void
Camera::TranslateAlongUp(
	float amt
) {
	glm::vec3 translation = up * amt;
	eye += translation;
	lookAt += translation;
	RecomputeAttributes();
}

Ray Camera::GenerateRay(int x, int y, int width, int height) const {

	float tan_fovy = tan(fov / 2 * DEG2RAD);
	float len = glm::length(lookAt - eye);
	float aspect = static_cast<float>(width) / static_cast<float>(height);
	glm::vec3 V = up * len * tan_fovy;
	glm::vec3 H = right * len * aspect * tan_fovy;

	float pixelNDCx = (2.0 * x / width - 1.0);
	float pixelNDCy = (1.0 - 2.0 * y / height);
	glm::vec3 P = lookAt + pixelNDCx * H + pixelNDCy * V;

	return Ray(eye, normalize(P - eye));
}
