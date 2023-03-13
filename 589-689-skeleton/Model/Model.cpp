#include "Model.h"

Model::Model(const std::string& texturePath) : texturePath(texturePath) {
	this->terrain = std::make_shared<FFS>();
	Log::debug("HERE 1");
	if (!this->hasTexture()) { this->texture = nullptr; return; }
	Log::debug("HERE 2");
	this->texture = std::make_shared<Texture>(Texture(texturePath, GL_LINEAR));
	this->texture->bind();
}

Model::Model(const std::string& fileName, const std::string& texturePath) : fileName(fileName), texturePath(texturePath) {
	this->process.cpuGeom = GeomLoaderForOBJ::loadIntoCPUGeometry(fileName);
	this->process.gpuGeom.bind();
	this->process.gpuGeom.setVerts(this->process.cpuGeom.verts);
	this->process.gpuGeom.setNormals(this->process.cpuGeom.normals);
	this->process.gpuGeom.setUVs(this->process.cpuGeom.uvs);
	if (!this->hasTexture()) { this->texture = nullptr; return; }
	this->texture = std::make_shared<Texture>(Texture(texturePath, GL_LINEAR));
	this->texture->bind();
}

bool Model::hasTexture() {
	if (this->terrain) {
		return !this->terrain->getFreeFormSurface().cpuGeom.uvs.empty() && !this->texturePath.empty();
	}
	return !this->process.cpuGeom.uvs.empty() && !this->texturePath.empty();
}

PhongLighting& Model::getPhongLighting() {
	return this->phongLighting;
}
std::shared_ptr<FFS> Model::getTerrain() {
	return this->terrain;
}

void Model::resetLightingToDefaults() {
	this->phongLighting.diffuseCol = glm::vec3(90.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f);
	this->phongLighting.lightPos = glm::vec3(0.0f, 35.0f, -35.0f);
	this->phongLighting.lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	this->phongLighting.ambientStrength = 0.035f;
	this->phongLighting.simpleWireframe = true;
	this->phongLighting.bIsChanging = true;
}

void Model::render() { glDrawArrays(GL_TRIANGLES, 0, GLsizei(this->process.cpuGeom.verts.size())); }
