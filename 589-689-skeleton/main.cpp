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
			{GLFW_KEY_E, false},
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
			if (this->camera.getCameraType() == CameraType::panMode) {
				this->camera.handleRotation(xpos - this->mouseOldX, ypos - this->mouseOldY);
			} else if (this->camera.getCameraType() == CameraType::rotationalMode) {
				camera.incrementTheta(ypos - this->mouseOldY);
				camera.incrementPhi(xpos - this->mouseOldX);
			}
		}
		this->mouseOldX = xpos;
		this->mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset) {
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		if (this->camera.getCameraType() == CameraType::rotationalMode) {
			camera.incrementR(yoffset);
		}
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
		glm::vec3 cameraPosition = (this->camera.getCameraType() == CameraType::panMode) ? this->camera.getOrientation().position : this->camera.getRotationalCameraPos();

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

		return intersection_point;
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
		glPolygonMode(GL_FRONT_AND_BACK, (model.getPhongLighting().simpleWireframe ? GL_LINE : GL_FILL));

		// Input
		if (inputManager->onKeyHeld(GLFW_KEY_W)) inputManager->getCamera().handleTranslation(GLFW_KEY_W);
		if (inputManager->onKeyHeld(GLFW_KEY_A)) inputManager->getCamera().handleTranslation(GLFW_KEY_A);
		if (inputManager->onKeyHeld(GLFW_KEY_S)) inputManager->getCamera().handleTranslation(GLFW_KEY_S);
		if (inputManager->onKeyHeld(GLFW_KEY_D)) inputManager->getCamera().handleTranslation(GLFW_KEY_D);

		if (inputManager->onKeyDown(GLFW_KEY_E)) model.getTerrain()->resetSelectedControlPoints();
		if (inputManager->onKeyDown(GLFW_KEY_R)) model.getTerrain()->resetSelectedWeights();

		glm::vec3 mousePosition3D = inputManager->getMousePosition3D();
		model.getTerrain()->detectControlPoints(mousePosition3D);

		if (inputManager->onKeyHeld(GLFW_MOUSE_BUTTON_LEFT)) {
			glm::vec2 mousePosition2D = inputManager->getMousePosition2D();
			float val = (model.getTerrain()->getBrushSettings().bIsRising) ? 0.1f : -0.1f;
			model.getTerrain()->updateControlPoints(glm::vec3(0.0f, val * model.getTerrain()->getBrushSettings().brushRateScale, 0.0f));
		}

		if (inputManager->isScrollingUp()) {
			model.getTerrain()->updateControlPoints(1.0f * model.getTerrain()->getNURBSSettings().weightRate);
		} else if (inputManager->isScrollingDown()) {
			model.getTerrain()->updateControlPoints(-1.0f * model.getTerrain()->getNURBSSettings().weightRate);
		}

		// ImGUI
		model.getPhongLighting().bIsChanging = false;
		model.getTerrain()->getTerrainSettings().bIsChanging = false;
		model.getTerrain()->getNURBSSettings().bIsChanging = false;
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Settings", (bool*)0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(700, 400));
		ImGui::SetWindowFontScale(1.10f);
		if (ImGui::BeginTabBar("Tab Bar")) {
			if (ImGui::BeginTabItem("Terrain Settings")) {
				ImGui::PushItemWidth(200);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::Text("Basic Terrain Settings:");
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				model.getTerrain()->getTerrainSettings().bIsChanging |= ImGui::SliderInt("Control Points", &model.getTerrain()->getTerrainSettings().nControlPoints, 6, 100);
				model.getTerrain()->getTerrainSettings().bIsChanging |= ImGui::SliderFloat("Terrain Size", &model.getTerrain()->getTerrainSettings().terrainSize, 10.0f, 100.0f);
				if (ImGui::Button("Reset to Defaults")) model.getTerrain()->resetTerrainToDefaults();
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::Text("Random Terrain Generation Settings:");
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::SliderFloat("Skip Probability", &model.getTerrain()->getRandomGenerationSettings().skipProbability, 0.0f, 1.0f);
				ImGui::SliderFloat("Min Height", &model.getTerrain()->getRandomGenerationSettings().minHeight, -10.0f, model.getTerrain()->getRandomGenerationSettings().maxHeight);
				ImGui::SliderFloat("Max Height", &model.getTerrain()->getRandomGenerationSettings().maxHeight, model.getTerrain()->getRandomGenerationSettings().minHeight, 30.0f);
				if (ImGui::Button("Random Generation")) model.getTerrain()->generateRandomTerrain();
				if (ImGui::Button("Reset RNG Settings")) model.getTerrain()->resetRandomGenerationSettings();
				for (int i = 0; i < 5; i++) ImGui::Spacing();
				if (ImGui::Button("Reset All")) {
					model.getTerrain()->resetAllSettings();
					model.resetLightingToDefaults();
				}
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::Text("Average %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("NURBS Surface Settings")) {
				ImGui::PushItemWidth(200);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				model.getTerrain()->getNURBSSettings().bIsChanging |= ImGui::SliderInt("Order u: ", &model.getTerrain()->getNURBSSettings().k_u, 2, model.getTerrain()->getGeneratedTerrain().generatedPoints.size());
				model.getTerrain()->getNURBSSettings().bIsChanging |= ImGui::SliderInt("Order v: ", &model.getTerrain()->getNURBSSettings().k_v, 2, model.getTerrain()->getGeneratedTerrain().generatedPoints[0].size());
				model.getTerrain()->getNURBSSettings().bIsChanging |= ImGui::SliderFloat("Resolution: ", &model.getTerrain()->getNURBSSettings().resolution, 10, 200);
				ImGui::SliderFloat("Weight Change Rate: ", &model.getTerrain()->getNURBSSettings().weightRate, 1.0f, 10.0f);
				ImGui::Checkbox("Display Control Points", &model.getTerrain()->getNURBSSettings().bDisplayControlPoints);
				ImGui::Checkbox("Display Line Segments", &model.getTerrain()->getNURBSSettings().bDisplayLineSegments);
				model.getTerrain()->getNURBSSettings().bIsChanging |= ImGui::Checkbox("Bezier", &model.getTerrain()->getNURBSSettings().bBezier);
				ImGui::Checkbox("Closed Loop", &model.getTerrain()->getNURBSSettings().bClosedLoop);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				if (ImGui::Button("Reset All Control Points")) model.getTerrain()->resetAllControlPoints();
				if (ImGui::Button("Reset All Weights")) model.getTerrain()->resetAllWeights();
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::Text("E - Reset Selected Control Points");
				ImGui::Text("R - Reset Selected Weights");
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				if (ImGui::Button("Reset to Defaults")) model.getTerrain()->resetNURBSToDefaults();
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Brush Settings")) {
				ImGui::PushItemWidth(200);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::SliderFloat("Radius of Brush: ", &model.getTerrain()->getBrushSettings().brushRadius, 0.1f, 10.5f);
				ImGui::SliderFloat("Brush Speed: ", &model.getTerrain()->getBrushSettings().brushRateScale, 0.5f, 10.0f);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::Checkbox("Rise/Lower", &model.getTerrain()->getBrushSettings().bIsRising);
				ImGui::Checkbox("Display Convex Hull", &model.getTerrain()->getBrushSettings().bDisplayConvexHull);
				ImGui::Checkbox("Planar/Normal", &model.getTerrain()->getBrushSettings().bIsPlanar);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				if (ImGui::Button("Reset to Defaults")) model.getTerrain()->resetBurshToDefaults();
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lighting Settings")) {
				ImGui::PushItemWidth(200);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				if (!model.hasTexture()) model.getPhongLighting().bIsChanging |= ImGui::ColorEdit3("Diffuse Colour", glm::value_ptr(model.getPhongLighting().diffuseCol));
				model.getPhongLighting().bIsChanging |= ImGui::DragFloat3("Light Position", glm::value_ptr(model.getPhongLighting().lightPos));
				model.getPhongLighting().bIsChanging |= ImGui::ColorEdit3("Light Colour", glm::value_ptr(model.getPhongLighting().lightCol));
				model.getPhongLighting().bIsChanging |= ImGui::SliderFloat("Ambient Atrength", &model.getPhongLighting().ambientStrength, 0.0f, 1.f);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::InputText("Texture", &model.getTextureSettings().texturePath[0], model.getTextureSettings().texturePath.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_EnterReturnsTrue);
				if (ImGui::IsItemClicked()) {
					window.openFile(model.getTextureSettings().texturePath, { { L"Image files", L"*.jpg;*.png;*.bmp" } });
					model.setTexture(model.getTextureSettings().texturePath);
				}
				if (model.hasTexture()) ImGui::Image((void*)model.getTextureSettings().texture2DRender.textureID, ImVec2(205.0f, 205.0f), ImVec2(0, 1), ImVec2(1, 0));
				if (ImGui::Button("Remove Texture")) model.removeTexture();
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				model.getPhongLighting().bIsChanging |= ImGui::Checkbox("Wireframe", &model.getPhongLighting().simpleWireframe);
				ImGui::Checkbox("Pan/Sphere", &inputManager->getCamera().bIsCameraChanging);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				if (ImGui::Button("Reset to Defaults")) model.resetLightingToDefaults();
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Export/Import Settings")) {
				ImGui::PushItemWidth(200);
				for (int i = 0; i < 3; i++) ImGui::Spacing();
				ImGui::InputText("Filename", &model.getExportImportSettings().exportFileName[0], model.getExportImportSettings().exportFileName.size());
				ImGui::InputText("Save Location", &model.getExportImportSettings().exportFileLocation[0], model.getExportImportSettings().exportFileLocation.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_EnterReturnsTrue);
				if (ImGui::IsItemClicked()) window.openDirectory(model.getExportImportSettings().exportFileLocation);
				for (int i = 0; i < 5; i++) ImGui::Spacing();
				if (ImGui::Button("Export Terrain"))
					model.getExportImportSettings().bDisplayAlert = model.exportToObj(model.getExportImportSettings().exportFileName, model.getExportImportSettings().exportFileLocation);
				if (model.getExportImportSettings().bDisplayAlert) ImGui::OpenPopup("Saved");
				if (ImGui::BeginPopupModal("Saved", &model.getExportImportSettings().bDisplayAlert, ImGuiWindowFlags_AlwaysAutoResize)) {
					for (int i = 0; i < 2; i++) ImGui::Spacing();
					ImGui::Text("Successfully saved %s.obj and %s.nobj to\n           %s", model.getExportImportSettings().exportFileName.c_str(), model.getExportImportSettings().exportFileName.c_str(), model.getExportImportSettings().exportFileLocation.c_str());
					for (int i = 0; i < 5; i++) ImGui::Spacing();
					ImVec2 contentSize = ImGui::GetContentRegionAvail();
					float buttonWidth = 125.0f;
					float buttonX = contentSize.x / 2.0f - buttonWidth / 2.0f;
					ImGui::SetCursorPosX(buttonX);
					if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
						model.getExportImportSettings().bDisplayAlert = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (ImGui::Button("Import Terrain")) {
					window.openFile(model.getExportImportSettings().importFileLocation, { { L"NUBRS files", L"*.nobj" } });
					model.importFromNObj(model.getExportImportSettings().importFileLocation);
				}
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		// Render
		shader.use();
		if (model.getPhongLighting().bIsChanging) inputManager->updateShadingUniforms(model);
		inputManager->viewPipeline();
		model.render();

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
