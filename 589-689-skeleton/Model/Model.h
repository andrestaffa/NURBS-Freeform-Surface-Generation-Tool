#pragma once

#include "FFS.h"
#include "../Texture.h"
#include "../GeomLoaderForOBJ.h"

struct ModelSettings {
	bool simpleWireframe = true;
};

struct PhongLighting {
	glm::vec3 lightPos = glm::vec3(0.0f, 35.0f, -35.0f);
	glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 diffuseCol = glm::vec3(1.0f, 0.0f, 0.0f);
	float ambientStrength = 0.035f;
	bool bIsChanging = false;
};

class Model {

private:

	const std::string fileName;
	const std::string texturePath;

	Process process;
	std::shared_ptr<FFS> terrain;
	std::shared_ptr<Texture> texture;

	PhongLighting phongLighting;
	ModelSettings modelSettings;

public:
	Model();
	Model(const std::string& fileName, const std::string& texturePath = "");

public:
	bool hasTexture();
	PhongLighting& getPhongLighting();
	ModelSettings& getModelSettings();
	std::shared_ptr<FFS> getTerrain();

	void render();
};
