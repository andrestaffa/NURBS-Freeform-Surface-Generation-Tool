#include "Model.h"
#include <stb/stb_image.h>

Model::Model(const std::string& texturePath, const std::string& fileLocation) {
	this->textureSettings.texturePath = texturePath;
	if (fileLocation.empty()) {
		this->exportImportSettings.exportFileName.resize(25, '\0');
		this->terrain = std::make_shared<FFS>();
		this->setTexture(texturePath);
		return;
	};
	this->exportImportSettings.exportFileName.resize(25, '\0');
	this->process.cpuGeom = GeomLoaderForOBJ::loadIntoCPUGeometry(fileLocation);
	this->process.gpuGeom.bind();
	this->process.gpuGeom.setVerts(this->process.cpuGeom.verts);
	this->process.gpuGeom.setNormals(this->process.cpuGeom.normals);
	this->process.gpuGeom.setUVs(this->process.cpuGeom.uvs);
	this->setTexture(texturePath);
}

void Model::render() {
	if (this->terrain) {
		this->terrain->render();
		return;
	}
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(this->process.cpuGeom.verts.size()));
}

// Texture Settings
bool Model::hasTexture() {
	if (this->terrain) {
		return !this->terrain->getFreeFormSurface().cpuGeom.uvs.empty() && !this->textureSettings.texturePath.empty();
	}
	return !this->process.cpuGeom.uvs.empty() && !this->textureSettings.texturePath.empty();
}

void Model::setTexture(const std::string& texturePath) {
	if (!this->hasTexture()) { this->textureSettings.texture = nullptr; return; }
	this->textureSettings.texturePath = texturePath;
	this->textureSettings.texture = std::make_shared<Texture>(Texture(texturePath, GL_LINEAR));
	this->textureSettings.texture->bind();
	this->setTexture2DRender(texturePath);
	this->phongLighting.bIsChanging = true;
}

void Model::removeTexture() {
	if (!this->textureSettings.texture || this->textureSettings.texturePath.empty()) return;
	this->textureSettings.texturePath = "";
	this->textureSettings.texture->unbind();
	this->textureSettings.texture = nullptr;
	glBindTexture(GL_TEXTURE_2D, this->textureSettings.texture2DRender.textureID);
	stbi_image_free(this->textureSettings.texture2DRender.imageData);
	this->textureSettings.texture2DRender.imageData = nullptr;
	this->phongLighting.bIsChanging = true;
}

void Model::setTexture2DRender(const std::string& texturePath) {
	if (!this->textureSettings.texture || this->textureSettings.texturePath.empty()) return;
	if (this->textureSettings.texture2DRender.imageData) {
		glBindTexture(GL_TEXTURE_2D, this->textureSettings.texture2DRender.textureID);
		stbi_image_free(this->textureSettings.texture2DRender.imageData);
		this->textureSettings.texture2DRender.imageData = nullptr;
	}
	int numComponents, width, height;
	this->textureSettings.texture2DRender.imageData = stbi_load(texturePath.c_str(), &width, &height, &numComponents, 0);
	GLuint format = GL_RGB;
	switch (numComponents) {
	case 4:
		format = GL_RGBA;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 2:
		format = GL_RG;
		break;
	case 1:
		format = GL_RED;
		break;
	default:
		break;
	};
	glGenTextures(1, &this->textureSettings.texture2DRender.textureID);
	glBindTexture(GL_TEXTURE_2D, this->textureSettings.texture2DRender.textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, this->textureSettings.texture2DRender.imageData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	this->textureSettings.texture2DRender.width = width;
	this->textureSettings.texture2DRender.width = height;
}

// Lighting Settings
void Model::resetLightingToDefaults() {
	this->phongLighting.diffuseCol = glm::vec3(90.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f);
	this->phongLighting.lightPos = glm::vec3(0.0f, 35.0f, -35.0f);
	this->phongLighting.lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	this->phongLighting.ambientStrength = 0.035f;
	this->phongLighting.simpleWireframe = false;
	this->removeTexture();
	this->phongLighting.bIsChanging = true;
}

// Export/Import Settings
bool Model::exportFileFormat(const char* format, const std::vector<std::string>& text, const std::string& filename, const std::string& dir) {
	if (dir.empty()) return false;
	char fullpath[1024];
	sprintf_s(fullpath, sizeof(fullpath), format, dir.c_str(), filename.c_str());
	FILE* file = fopen(fullpath, "w");
	if (file) {
		for (const std::string& objStr : text) {
			fwrite(objStr.c_str(), sizeof(char), objStr.length(), file);
			fputc('\n', file);
		}
		fclose(file);
	}
	return file != nullptr;
}

bool Model::exportToObj(const std::string& filename, const std::string& dir) {
	if (!this->terrain) return false;

	std::vector<std::string> nObjs = this->terrain->getExportNObjFormat();
	if (!this->textureSettings.texturePath.empty()) nObjs.push_back("texture " + this->textureSettings.texturePath);
	std::vector<std::string> objs = this->terrain->getExportObjFormat();

	bool nurbsObjFileFlag = this->exportFileFormat("%s/%s.nobj", nObjs, filename, dir);
	bool objFileFlag = this->exportFileFormat("%s/%s.obj", objs, filename, dir);

	return nurbsObjFileFlag && objFileFlag;
}

bool Model::importFromNObj(const std::string& path) {
	std::vector<glm::vec3> controlPoints;
	std::vector<float> weights;
	int k_u = 3, k_v = 3;
	float resolution = 100.0f;
	int nControlPoints = 20;
	float terrainSize = 10.0f;
	std::string texturePath(200, '\0');

	FILE* file = fopen(path.c_str(), "r");
	if (file == NULL) {
		Log::error("Cannot open File");
		return false;
	}

	int val = -1;
	while (true) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;
		if (strcmp(lineHeader, "cp") == 0) {
			glm::vec3 controlPoint;
			float weight;
			val = fscanf(file, "%f %f %f %f\n", &controlPoint.x, &controlPoint.y, &controlPoint.z, &weight);
			controlPoints.push_back(controlPoint);
			weights.push_back(weight);
		} else if (strcmp(lineHeader, "ku") == 0) {
			val = fscanf(file, "%d\n", &k_u);
		} else if (strcmp(lineHeader, "kv") == 0) {
			val = fscanf(file, "%d\n", &k_v);
		} else if (strcmp(lineHeader, "r") == 0) {
			val = fscanf(file, "%f\n", &resolution);
		} else if (strcmp(lineHeader, "nCp") == 0) {
			val = fscanf(file, "%d\n", &nControlPoints);
		} else if (strcmp(lineHeader, "tz") == 0) {
			val = fscanf(file, "%f\n", &terrainSize);
		} else if (strcmp(lineHeader, "texture") == 0) {
			fgets(texturePath.data(), texturePath.size(), file);
			size_t len = strlen(texturePath.data());
			if (len > 0 && texturePath.data()[len - 1] == '\n') {
				texturePath.data()[len - 1] = '\0';
			}
		}
	}
	texturePath.resize(strlen(texturePath.data()));
	texturePath.erase(0, 1);

	ImportNOBJSettings settings;
	settings.controlPoints = controlPoints;
	settings.weights = weights;
	settings.k_u = k_u;
	settings.k_v = k_v;
	settings.resolution = resolution;
	settings.nControlPoints = nControlPoints;
	settings.terrainSize = terrainSize;

	bool success = this->terrain->getImportNObjFormat(settings);
	if (success) {
		this->textureSettings.texturePath = texturePath;
		this->setTexture(this->textureSettings.texturePath);
	}
	return false;
}
