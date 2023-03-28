#include "Camera.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "glm/gtc/matrix_transform.hpp"

Camera::Camera(float aspect, const glm::vec3& position, const glm::vec3& up, const glm::vec3& front) {
	this->orientation.position = position;
	this->orientation.up = up;
	this->orientation.front = front;
	this->aspect = aspect;
	this->cameraTranslateSens = 0.05f;
	this->cameraRotationSens = 0.05f;
	this->yaw = -54.95f;
	this->pitch = -30.649965f;
	this->handleRotation(yaw, pitch);
	this->rotationalCameraProperties.theta = glm::radians(45.f);
	this->rotationalCameraProperties.phi = glm::radians(45.f);
	this->rotationalCameraProperties.radius = 3.0f;
}

glm::mat4 Camera::getView() {
	this->cameraType = (this->bIsCameraChanging) ? CameraType::rotationalMode : CameraType::panMode;
	if (this->cameraType == CameraType::panMode) {
		return glm::lookAt(this->orientation.position, this->orientation.position + this->orientation.front, this->orientation.up);
	} else if (this->cameraType == CameraType::rotationalMode) {
		glm::vec3 eye = this->rotationalCameraProperties.radius * glm::vec3(glm::cos(this->rotationalCameraProperties.theta) * glm::sin(this->rotationalCameraProperties.phi), glm::sin(this->rotationalCameraProperties.theta), glm::cos(this->rotationalCameraProperties.theta) * glm::cos(this->rotationalCameraProperties.phi));
		glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		return glm::lookAt(eye, at, up);
	}
}

glm::mat4 Camera::getPerspective() {
	return glm::perspective(glm::radians(45.0f), this->aspect, 0.01f, 1000.f);
}

Orientation Camera::getOrientation() {
	return this->orientation;
}

glm::vec3 Camera::getRotationalCameraPos() {
	return this->rotationalCameraProperties.radius * glm::vec3(glm::cos(this->rotationalCameraProperties.theta) * glm::sin(this->rotationalCameraProperties.phi), glm::sin(this->rotationalCameraProperties.theta), glm ::cos(this->rotationalCameraProperties.theta) * glm::cos(this->rotationalCameraProperties.phi));
}

void Camera::handleRotation(float xoffset, float yoffset) {
	xoffset *= this->cameraRotationSens;
	yoffset *= this->cameraRotationSens;

	this->yaw += xoffset;
	this->pitch -= yoffset;

	glm::vec3 front;
	front.x = glm::cos(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch));
	front.y = glm::sin(glm::radians(this->pitch));
	front.z = glm::sin(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch));
	this->orientation.front = glm::normalize(front);
}

void Camera::handleTranslation(int key) {
	if (key == GLFW_KEY_W)
		this->orientation.position += this->cameraTranslateSens * this->orientation.front;
	if (key == GLFW_KEY_A)
		this->orientation.position -= glm::normalize(glm::cross(this->orientation.front, this->orientation.up)) * this->cameraTranslateSens;
	if (key == (GLFW_KEY_S))
		this->orientation.position -= this->cameraTranslateSens * this->orientation.front;
	if (key == GLFW_KEY_D)
		this->orientation.position += glm::normalize(glm::cross(this->orientation.front, this->orientation.up)) * this->cameraTranslateSens;
}

void Camera::incrementTheta(float dt) {
	if (this->rotationalCameraProperties.theta + (dt / 100.0f) < M_PI_2 && this->rotationalCameraProperties.theta + (dt / 100.0f) > -M_PI_2) {
		this->rotationalCameraProperties.theta += dt / 100.0f;
	}
}

void Camera::incrementPhi(float dp) {
	this->rotationalCameraProperties.phi -= dp / 100.0f;
	if (this->rotationalCameraProperties.phi > 2.0 * M_PI) {
		this->rotationalCameraProperties.phi -= 2.0 * M_PI;
	}
	else if (this->rotationalCameraProperties.phi < 0.0f) {
		this->rotationalCameraProperties.phi += 2.0 * M_PI;
	}
}

void Camera::incrementR(float dr) {
	this->rotationalCameraProperties.radius -= dr;
}
