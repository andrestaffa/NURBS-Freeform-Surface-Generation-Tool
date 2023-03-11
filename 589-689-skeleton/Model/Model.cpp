#include "Model.h"

Model::Model() {
	this->terrain = std::make_shared<FFS>();
}

Model::Model(const std::string& fileName, const std::string& texturePath) :
	fileName(fileName),
	texturePath(texturePath) {
	this->process.cpuGeom = GeomLoaderForOBJ::loadIntoCPUGeometry(fileName);
	this->process.gpuGeom.bind();
	this->process.gpuGeom.setVerts(this->process.cpuGeom.verts);
	this->process.gpuGeom.setNormals(this->process.cpuGeom.normals);
	this->process.gpuGeom.setUVs(this->process.cpuGeom.uvs);
	if (!this->hasTexture()) { this->texture = nullptr; return; }
	this->texture = std::make_shared<Texture>(Texture(texturePath, GL_LINEAR));
	this->texture->bind();
}

bool Model::hasTexture() { return this->process.cpuGeom.uvs.size() > 0 && !this->texturePath.empty(); }
PhongLighting& Model::getPhongLighting() { return this->phongLighting; }
ModelSettings& Model::getModelSettings() { return this->modelSettings; }
std::shared_ptr<FFS> Model::getTerrain() { return this->terrain; }

void Model::render() { glDrawArrays(GL_TRIANGLES, 0, GLsizei(this->process.cpuGeom.verts.size())); }
