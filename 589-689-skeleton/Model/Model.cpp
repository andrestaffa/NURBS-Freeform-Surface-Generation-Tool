#include "Model.h"

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
	this->phongLighting.bIsChanging = true;
}

void Model::removeTexture() {
	if (!this->textureSettings.texture || this->textureSettings.texturePath.empty()) return;
	this->textureSettings.texturePath = "";
	this->textureSettings.texture->unbind();
	this->textureSettings.texture = nullptr;
	this->phongLighting.bIsChanging = true;
}

// Lighting Settings
void Model::resetLightingToDefaults() {
	this->phongLighting.diffuseCol = glm::vec3(90.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f);
	this->phongLighting.lightPos = glm::vec3(0.0f, 35.0f, -35.0f);
	this->phongLighting.lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	this->phongLighting.ambientStrength = 0.035f;
	this->phongLighting.simpleWireframe = true;
	this->removeTexture();
	this->phongLighting.bIsChanging = true;
}

// Export/Import Settings
bool Model::exportToObj(const std::string& filename, const std::string& dir) {
	if (dir.empty()) return false;
	if (!this->terrain) return false;
	std::vector<std::string> objs = this->terrain->getExportObjFormat();
	char fullpath[1024];
	sprintf_s(fullpath, sizeof(fullpath), "%s/%s.obj", dir.c_str(), filename.c_str());
	FILE* file = fopen(fullpath, "w");
	if (file) {
		for (const std::string& objStr : objs) {
			fwrite(objStr.c_str(), sizeof(char), objStr.length(), file);
			fputc('\n', file);
		}
		fclose(file);
	}
	return file != nullptr;
}
