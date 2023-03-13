#pragma once

#include "FFS.h"
#include "../Texture.h"
#include "../GeomLoaderForOBJ.h"


struct PhongLighting {
	glm::vec3 diffuseCol = glm::vec3(90.0f/255.0f, 87.0f/255.0f, 87.0f/255.0f);
	glm::vec3 lightPos = glm::vec3(0.0f, 35.0f, -35.0f);
	glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	float ambientStrength = 0.035f;
	bool simpleWireframe = true;
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

public:
	Model(const std::string& texturePath = "");
	Model(const std::string& fileName, const std::string& texturePath);

public:
	bool hasTexture();
	PhongLighting& getPhongLighting();
	std::shared_ptr<FFS> getTerrain();

	void resetLightingToDefaults();

	void render();
};
