#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>
#include <unordered_map>

#include "Window.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Camera.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Model/Model.h"

class InputManager : public CallbackInterface {

private:
	ShaderProgram& shader;

	glm::ivec2 screenDim;
	glm::vec2 screenPos;

	std::unordered_map<int, bool> pressedKeys;
	std::unordered_map<int, bool> heldKeys;

	Camera camera;
	double mouseOldX, mouseOldY;

	bool bIsScrollingUp;
	bool bIsScrollingDown;

public:
	InputManager(ShaderProgram& shader, int screenWidth, int screenHeight) :
		shader(shader),
		camera((float)screenWidth / (float)screenHeight, glm::vec3(-13.001894f, 10.81906f, 16.41005f)),
		screenDim(screenWidth, screenHeight),
		mouseOldX(-1.0),
		mouseOldY(-1.0),
		bIsScrollingUp(false),
		bIsScrollingDown(false)
	{
		this->pressedKeys = std::unordered_map<int, bool>({
			{GLFW_KEY_R, false},
		});
		this->heldKeys = std::unordered_map <int, bool>({
			{GLFW_KEY_W, false},
			{GLFW_KEY_A, false},
			{GLFW_KEY_S, false},
			{GLFW_KEY_D, false},
			{GLFW_MOUSE_BUTTON_LEFT, false}
		});
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		this->pressedKeys[key] = action == GLFW_PRESS;
		this->heldKeys[key] = action == GLFW_REPEAT || action == GLFW_PRESS;
	}

	virtual void mouseButtonCallback(int button, int action, int mods) {
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		this->pressedKeys[button] = action == GLFW_PRESS;
		this->heldKeys[button] = action == GLFW_PRESS;
	}	

	virtual void cursorPosCallback(double xpos, double ypos) {
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		this->screenPos.x = (float)xpos;
		this->screenPos.y = (float)ypos;
		if (this->onKeyHeld(GLFW_MOUSE_BUTTON_RIGHT)) {
			this->camera.handleRotation(xpos - this->mouseOldX, ypos - this->mouseOldY);
		}
		this->mouseOldX = xpos;
		this->mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset) {
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		this->bIsScrollingUp = (yoffset > 0);
		this->bIsScrollingDown = (yoffset < 0);
	}

	virtual void windowSizeCallback(int width, int height) {
		CallbackInterface::windowSizeCallback(width, height);
		this->screenDim = glm::ivec2(width, height);
		glViewport(0, 0, width, height);
	}

	void refreshInput() {
		for (auto& it : this->pressedKeys) it.second = false;
		this->bIsScrollingUp = false;
		this->bIsScrollingDown = false;
	}

	bool onKeyDown(int key) {
		return this->pressedKeys[key];
	}

	bool onKeyHeld(int key) {
		return this->heldKeys[key];
	}

	bool isScrollingUp() const {
		return this->bIsScrollingUp;
	}

	bool isScrollingDown() const {
		return this->bIsScrollingDown;
	}


	void viewPipeline(bool bIdentity = false) {
		glm::mat4 M = glm::mat4(1.0);
		glm::mat4 V = (bIdentity) ? glm::mat4(1.0f) : this->camera.getView();
		glm::mat4 P = (bIdentity) ? glm::mat4(1.0f) : this->camera.getPerspective();
		glUniformMatrix4fv(glGetUniformLocation(this->shader, "M"), 1, GL_FALSE, glm::value_ptr(M));
		glUniformMatrix4fv(glGetUniformLocation(this->shader, "V"), 1, GL_FALSE, glm::value_ptr(V));
		glUniformMatrix4fv(glGetUniformLocation(this->shader, "P"), 1, GL_FALSE, glm::value_ptr(P));
	}

	void updateShadingUniforms(Model& model) {
		PhongLighting phongLighting = model.getPhongLighting();
		glUniform3f(glGetUniformLocation(this->shader, "lightPos"), phongLighting.lightPos.x, phongLighting.lightPos.y, phongLighting.lightPos.z);
		glUniform3f(glGetUniformLocation(this->shader, "lightCol"), phongLighting.lightCol.r, phongLighting.lightCol.g, phongLighting.lightCol.b);
		glUniform3f(glGetUniformLocation(this->shader, "diffuseCol"), phongLighting.diffuseCol.r, phongLighting.diffuseCol.g, phongLighting.diffuseCol.b);
		glUniform1f(glGetUniformLocation(this->shader, "ambientStrength"), phongLighting.ambientStrength);
		glUniform1i(glGetUniformLocation(this->shader, "texExistence"), (int)model.hasTexture());
	}

	glm::vec2 getMousePosition2D() {
		glm::vec2 startingVec = this->screenPos;
		glm::vec2 shiftedVec = startingVec + glm::vec2(0.5f, 0.5f);
		glm::vec2 scaledVec = shiftedVec / glm::vec2(screenDim);
		glm::vec2 reversedY = glm::vec2(scaledVec.x, 1.0f - scaledVec.y);
		glm::vec2 finalVec = reversedY * 2.0f - glm::vec2(1.0f, 1.0f);
		return finalVec;
	}

	glm::vec3 getMousePosition3D() {
		float width = this->screenDim.x;
		float height = this->screenDim.y;

		glm::mat4 projectionMatrix = this->camera.getPerspective();
		glm::mat4 viewMatrix = this->camera.getView();
		glm::vec3 cameraPosition = this->camera.getOrientation().position;

		float x = (2.0f * this->screenPos.x) / width - 1.0f;
		float y = 1.0f - (2.0f * this->screenPos.y) / height;
		float z = 1.0f;

		// Compute the ray in clip space
		glm::vec3 ray_nds = glm::vec3(x, y, z);
		glm::vec4 ray_clip = glm::vec4(glm::vec2(ray_nds.x, ray_nds.y), -1.0, 1.0);

		// Compute the ray in eye space
		glm::vec4 ray_eye = glm::inverse(projectionMatrix) * ray_clip;
		ray_eye = glm::vec4(glm::vec2(ray_eye.x, ray_eye.y), -1.0, 0.0);

		// Compute the ray in world space
		glm::vec3 ray_wor = glm::vec3(glm::inverse(viewMatrix) * ray_eye);
		ray_wor = glm::normalize(ray_wor);

		// Shift the ray origin by the camera position
		glm::vec3 ray_origin = cameraPosition;

		// Compute the intersection point with the x-z plane at y=0
		float t = -ray_origin.y / ray_wor.y;
		glm::vec3 intersection_point = ray_origin + t * ray_wor;
		//intersection_point.y = 0;

		return intersection_point;

		// NORMAL BASED PROJECTION

		//// Calculate intersection point with plane
		//float t = glm::dot((surfacePoint - cameraPosition), surfaceNormal) / glm::dot(ray_wor, surfaceNormal);
		//glm::vec3 intersectionPoint = cameraPosition + ray_wor * t;

		//// Modify y component based on surface normal
		//intersectionPoint.y += glm::dot(ray_wor, surfaceNormal);

		//return intersectionPoint;
	}

	Camera& getCamera() { return this->camera; }

};

int main() {
	Log::debug("Starting main");

	glfwInit();
	Window window(2560, 1440, "CPSC 589/689 - Project");
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(shader, window.getWidth(), window.getHeight());
	window.setCallbacks(inputManager);
	window.setupImGui();

	Model model = Model();

	shader.use();
	inputManager->updateShadingUniforms(model);

	while (!window.shouldClose()) {
		inputManager->refreshInput();
		glfwPollEvents();

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, (model.getModelSettings().simpleWireframe ? GL_LINE : GL_FILL));

		// Input
		if (inputManager->onKeyHeld(GLFW_KEY_W)) inputManager->getCamera().handleTranslation(GLFW_KEY_W);
		if (inputManager->onKeyHeld(GLFW_KEY_A)) inputManager->getCamera().handleTranslation(GLFW_KEY_A);
		if (inputManager->onKeyHeld(GLFW_KEY_S)) inputManager->getCamera().handleTranslation(GLFW_KEY_S);
		if (inputManager->onKeyHeld(GLFW_KEY_D)) inputManager->getCamera().handleTranslation(GLFW_KEY_D);

		glm::vec3 mousePosition3D = inputManager->getMousePosition3D();
		model.getTerrain()->detectControlPoints(mousePosition3D);

		if (inputManager->onKeyHeld(GLFW_MOUSE_BUTTON_LEFT)) {
			glm::vec2 mousePosition2D = inputManager->getMousePosition2D();
			model.getTerrain()->updateControlPoints(glm::vec3(0.0f, 0.1f, 0.0f));
		}

		if (inputManager->isScrollingUp()) {
			model.getTerrain()->updateControlPoints(1.0f);
		} else if (inputManager->isScrollingDown()) {
			model.getTerrain()->updateControlPoints(-1.0f);
		}

		// ImGUI
		model.getPhongLighting().bIsChanging = false;
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Settings");
		if (!model.hasTexture()) model.getPhongLighting().bIsChanging |= ImGui::ColorEdit3("Diffuse colour", glm::value_ptr(model.getPhongLighting().diffuseCol));
		model.getPhongLighting().bIsChanging |= ImGui::DragFloat3("Light's position", glm::value_ptr(model.getPhongLighting().lightPos));
		model.getPhongLighting().bIsChanging |= ImGui::ColorEdit3("Light's colour", glm::value_ptr(model.getPhongLighting().lightCol));
		model.getPhongLighting().bIsChanging |= ImGui::SliderFloat("Ambient strength", &model.getPhongLighting().ambientStrength, 0.0f, 1.f);
		model.getPhongLighting().bIsChanging |= ImGui::Checkbox("Simple wireframe", &model.getModelSettings().simpleWireframe);
		ImGui::Text("Average %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		// Render
		shader.use();
		if (model.getPhongLighting().bIsChanging) inputManager->updateShadingUniforms(model);
		inputManager->viewPipeline();
		model.getTerrain()->render();

		glDisable(GL_FRAMEBUFFER_SRGB);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		window.swapBuffers();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}