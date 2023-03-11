#include "Camera.h"

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
}

glm::mat4 Camera::getView() {
	return glm::lookAt(this->orientation.position, this->orientation.position + this->orientation.front, this->orientation.up);
}

glm::mat4 Camera::getPerspective() {
	return glm::perspective(glm::radians(45.0f), this->aspect, 0.01f, 1000.f);
}

Orientation Camera::getOrientation() {
	return this->orientation;
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
