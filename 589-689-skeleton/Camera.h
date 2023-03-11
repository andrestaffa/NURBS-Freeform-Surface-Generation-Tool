#pragma once

//------------------------------------------------------------------------------
// This file contains an implementation of a spherical camera
//------------------------------------------------------------------------------

//#include <GL/glew.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct Orientation {
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
};

class Camera {
public:

	Camera(float aspect = 16.0f / 9.0f, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), const glm::vec3& front = glm::vec3(0.0f, 0.0f, -1.0f));

	glm::mat4 getView();
	glm::mat4 getPerspective();
	Orientation getOrientation();
	void handleRotation(float xoffset, float yoffset);
	void handleTranslation(int key);

private:

	Orientation orientation;

	float cameraTranslateSens;
	float cameraRotationSens;
	float yaw;
	float pitch;

	float aspect;


};
