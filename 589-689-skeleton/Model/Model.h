#pragma once

#include <cstdio>

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

struct TextureSettings {
	std::string texturePath = "";
	std::shared_ptr<Texture> texture = nullptr;
};

struct ExportImportSettings {
	std::string exportFileName = "";
	std::string exportFileLocation = "";
	std::string importFileLocation = "";
	bool bDisplayAlert = false;
};

class Model {

private:

	Process process;
	std::shared_ptr<FFS> terrain = nullptr;

	// Texture Settings
	TextureSettings textureSettings;
	// Lighting Settings
	PhongLighting phongLighting;
	// Export/Import Settings
	ExportImportSettings exportImportSettings;

public:

	Model(const std::string& texturePath = "", const std::string& fileLocation = "");

public:

	void render();

public:

	// Terrain
	std::shared_ptr<FFS> getTerrain() { return this->terrain; }

	// Texture Settings
	bool hasTexture();
	void setTexture(const std::string& texturePath);
	void removeTexture();
	TextureSettings& getTextureSettings() { return this->textureSettings; }

	// Lighting Settings
	void resetLightingToDefaults();
	PhongLighting& getPhongLighting() { return this->phongLighting; }

	// Export/Import Settings
	bool exportToObj(const std::string& filename, const std::string& dir);
	ExportImportSettings& getExportImportSettings() { return this->exportImportSettings; }
;
};
