#include "FFS.h"

FFS::FFS() {
	this->generatedTerrain.generatedPoints = this->generateControlPoints();
	this->generatedTerrain.weights = this->generateWeights(this->generatedTerrain.generatedPoints.size(), this->generatedTerrain.generatedPoints[0].size());
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::render() {
	if (this->terrainSettings.bIsChanging) this->createTerrain();
	if (this->nurbsSettings.bIsChanging) this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
	if (this->nurbsSettings.bDisplayControlPoints) {
		glPointSize(10.0f);
		this->controlPoints.gpuGeom.bind();
		glDrawArrays(GL_POINTS, 0, GLsizei(this->controlPoints.cpuGeom.verts.size()));
	}
	glLineWidth(10.0f);
	if (this->nurbsSettings.bDisplayLineSegments) {
		this->nurbsLines.gpuGeom.bind();
		glDrawArrays(GL_LINE_STRIP, 0, GLsizei(this->nurbsLines.cpuGeom.verts.size()));
	}
	this->freeFormSurface.gpuGeom.bind();
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(this->freeFormSurface.cpuGeom.verts.size()));
}

// NURBS
std::vector<float> FFS::generateKnotSequence(int length, int k) {
	int numControlPoints = length;
	int order = k;
	int numKnots = length + k;
	std::vector<float> U(numKnots);
	for (int i = 0; i < order; i++) {
		U[i] = 0.0f;
	}
	if (!this->nurbsSettings.bBezier) {
		for (int i = order; i < numKnots - order; i++) {
			U[i] = (float)(i - order + 1) / (float)(numControlPoints - order + 1);
		}
	}
	for (int i = numKnots - order; i < numKnots; i++) {
		U[i] = 1.0f;
	}
	return U;
}

std::vector<std::vector<float>> FFS::generateWeights(int u_length, int v_length) {
	std::vector<std::vector<float>> W(u_length, std::vector<float>(v_length));
	for (int i = 0; i < u_length; i++) {
		for (int j = 0; j < v_length; j++) {
			W[i][j] = 1.0f;
		}
	}
	return W;
}
int FFS::delta(const std::vector<float>& U, float u, int k, int m) {
	int i;
	for (i = 0; i <= m + k - 1; i++) {
		if (u >= U[i] && u < U[i + 1]) break;
	}
	int multiplicity = 0;
	for (int j = i + 1; j <= m + k; j++) {
		if (U[j] != U[i]) break;
		multiplicity++;
	}
	return i + multiplicity;
}

glm::vec3 FFS::FFS_NURBS(const std::vector<std::vector<glm::vec3>>& P, const std::vector<float>& U, const std::vector<float>& V, std::vector<std::vector<float>> W, float u, float v, int k_u, int k_v, int m) {
	std::vector<glm::vec3> D(k_v), C(k_u);
	std::vector<float> NV(k_v);
	int d = delta(U, u, k_u, m);
	int d_prime = delta(V, v, k_v, m);
	for (int i = 0; i <= k_u - 1; i++) {
		for (int j = 0; j <= k_v - 1; j++) {
			D[j] = P[d - i][d_prime - j] * W[d - i][d_prime - j];
			NV[j] = W[d - i][d_prime - j];
		}
		for (int r = k_v; r >= 2; r--) {
			int x = d_prime;
			for (int s = 0; s <= r - 2; s++) {
				float omega = (v - V[x]) / (V[x + r - 1] - V[x]);
				D[s] = (omega * D[s]) + ((1.0f - omega) * D[s + 1]);
				NV[s] = (omega * NV[s]) + ((1.0f - omega) * NV[s + 1]);
				x = x - 1;
			}
		}
		C[i] = D[0] / NV[0];
	}
	for (int r = k_u; r >= 2; r--) {
		int i = d;
		for (int s = 0; s <= r - 2; s++) {
			float omega = (u - U[i]) / (U[i + r - 1] - U[i]);
			C[s] = (omega * C[s]) + ((1.0f - omega) * C[s + 1]);
			i = i - 1;
		}
	}
	return C[0];
}


// FFS
std::vector<std::vector<glm::vec3>> FFS::generateControlPoints() {
	std::vector<std::vector<glm::vec3>> P(this->terrainSettings.nControlPoints, std::vector<glm::vec3>(this->terrainSettings.nControlPoints));

	float x_min = -this->terrainSettings.terrainSize;
	float x_max = this->terrainSettings.terrainSize;
	float z_min = -this->terrainSettings.terrainSize;
	float z_max = this->terrainSettings.terrainSize;

	float x_step = (x_max - x_min) / (P.size() - 1);
	float z_step = (z_max - z_min) / (P[0].size() - 1);

	for (int i = 0; i < P.size(); i++) {
		for (int j = 0; j < P[i].size(); j++) {
			float x = x_min + i * x_step;
			float z = z_min + j * z_step;
			P[i][j] = glm::vec3(x, 0.0f, z);
		}
	}

	return P;
}

void FFS::generateTerrain(const std::vector<std::vector<glm::vec3>>& P, const std::vector<std::vector<float>>& W) {
	this->generatedTerrain.Q.clear();

	std::vector<float> U = this->generateKnotSequence(P.size(), this->nurbsSettings.k_u);
	std::vector<float> V = this->generateKnotSequence(P[0].size(), this->nurbsSettings.k_v);

	int i = 0;
	for (float u = U[this->nurbsSettings.k_u - 1]; u <= U[P.size() + 1]; u += (1.0f / this->nurbsSettings.resolution)) {
		this->generatedTerrain.Q.push_back(std::vector<glm::vec3>());
		for (float v = V[this->nurbsSettings.k_v - 1]; v <= V[P[0].size() + 1]; v += (1.0f / this->nurbsSettings.resolution)) {
			this->generatedTerrain.Q[i].push_back(this->FFS_NURBS(P, U, V, W, u, v, this->nurbsSettings.k_u, this->nurbsSettings.k_v, P.size()));
		}
		i += 1;
	} 

	std::vector<glm::vec3> surfacePoints = this->generateQuads(this->generatedTerrain.Q);
	std::vector<glm::vec2> surfaceUVs = this->generateTextureCoord(this->generatedTerrain.Q);
	std::vector<glm::vec3> surfaceNormals = this->generateNormals(this->generatedTerrain.Q);
	std::vector<glm::vec3> quadPoints = this->generateQuads(P);

	// Surface
	this->freeFormSurface.gpuGeom.bind();
	this->freeFormSurface.cpuGeom.verts = surfacePoints;
	this->freeFormSurface.cpuGeom.uvs = surfaceUVs;
	this->freeFormSurface.cpuGeom.normals = surfaceNormals;
	this->freeFormSurface.gpuGeom.setVerts(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.gpuGeom.setUVs(this->freeFormSurface.cpuGeom.uvs);
	this->freeFormSurface.gpuGeom.setNormals(this->freeFormSurface.cpuGeom.normals);

	// Control Points
	this->controlPoints.gpuGeom.bind();
	this->controlPoints.cpuGeom.verts = quadPoints;
	this->controlPoints.cpuGeom.cols.resize(this->controlPoints.cpuGeom.verts.size(), glm::vec3(1.0f, 0.0f, 0.0f));
	this->controlPoints.gpuGeom.setVerts(this->controlPoints.cpuGeom.verts);
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);

	// NURBS Lines
	this->nurbsLines.gpuGeom.bind();
	this->nurbsLines.cpuGeom.verts = quadPoints;
	this->nurbsLines.cpuGeom.cols.resize(this->nurbsLines.cpuGeom.verts.size(), glm::vec3(0.0f, 1.0f, 0.0f));
	this->nurbsLines.gpuGeom.setVerts(this->nurbsLines.cpuGeom.verts);
	this->nurbsLines.gpuGeom.setCols(this->nurbsLines.cpuGeom.cols);

}

std::vector<glm::vec3> FFS::generateQuads(const std::vector<std::vector<glm::vec3>>& points) {
	std::vector<glm::vec3> R;
	for (size_t i = 0; i < points.size() - 1; i++) {
		for (size_t j = 0; j < points[i].size() - 1; j++) {
			R.push_back(points[i][j + 1]);
			R.push_back(points[i][j]);
			R.push_back(points[i + 1][j]);
			R.push_back(points[i][j + 1]);
			R.push_back(points[i + 1][j + 1]);
			R.push_back(points[i + 1][j]);
		}
	}
	return R;
}

std::vector<glm::vec2> FFS::generateQuads(const std::vector<std::vector<glm::vec2>>& points) {
	std::vector<glm::vec2> R;
	for (size_t i = 0; i < points.size() - 1; i++) {
		for (size_t j = 0; j < points[i].size() - 1; j++) {
			R.push_back(points[i][j + 1]);
			R.push_back(points[i][j]);
			R.push_back(points[i + 1][j]);
			R.push_back(points[i][j + 1]);
			R.push_back(points[i + 1][j + 1]);
			R.push_back(points[i + 1][j]);
		}
	}
	return R;
}

std::vector<glm::vec2> FFS::generateTextureCoord(const std::vector<std::vector<glm::vec3>>& controlPoints) {
	std::vector<std::vector<glm::vec2>> textureCoordinates;
	const int width = controlPoints.size();
	const int height = controlPoints[0].size();
	const float maxVal = std::max(width, height);
	for (int x = 0; x < width; ++x) {
		std::vector<glm::vec2> row;
		for (int y = 0; y < height; ++y) {
			const float u = (x + 0.5f) / maxVal;
			const float v = (y + 0.5f) / maxVal;
			row.push_back(glm::vec2(u, v));
		}
		textureCoordinates.push_back(row);
	}
	std::vector<glm::vec2> T = this->generateQuads(textureCoordinates);
	return T;
}

std::vector<glm::vec3> FFS::generateNormals(const std::vector<std::vector<glm::vec3>>& controlPoints) {
	std::vector<std::vector<glm::vec3>> normals;
	int numRows = controlPoints.size();
	int numCols = controlPoints[0].size();
	normals.resize(numRows);
	for (int i = 0; i < numRows; i++) {
		normals[i].resize(numCols);
	}
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			glm::vec3 normal(0.0f, 0.0f, 0.0f);
			if (i > 0 && j > 0) {
				glm::vec3 v1 = controlPoints[i][j] - controlPoints[i][j - 1];
				glm::vec3 v2 = controlPoints[i][j] - controlPoints[i - 1][j];
				normal += glm::cross(v1, v2);
			}
			if (i > 0 && j < numCols - 1) {
				glm::vec3 v1 = controlPoints[i][j] - controlPoints[i - 1][j];
				glm::vec3 v2 = controlPoints[i][j] - controlPoints[i][j + 1];
				normal += glm::cross(v1, v2);
			}
			if (i < numRows - 1 && j < numCols - 1) {
				glm::vec3 v1 = controlPoints[i][j] - controlPoints[i][j + 1];
				glm::vec3 v2 = controlPoints[i][j] - controlPoints[i + 1][j];
				normal += glm::cross(v1, v2);
			}
			if (i < numRows - 1 && j > 0) {
				glm::vec3 v1 = controlPoints[i][j] - controlPoints[i + 1][j];
				glm::vec3 v2 = controlPoints[i][j] - controlPoints[i][j - 1];
				normal += glm::cross(v1, v2);
			}
			normals[i][j] = glm::normalize(normal);
		}
	}
	std::vector<glm::vec3> N = this->generateQuads(normals);
	return N;
}

// .obj Formatting

std::vector<std::string> FFS::generateObjVertices(const std::vector<std::vector<glm::vec3>>& controlPoints) {
	std::vector<std::string> vertices;
	int numRows = controlPoints.size();
	int numCols = controlPoints[0].size();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const glm::vec3& cp = controlPoints[i][j];
			std::string vertexStr = "v " + std::to_string(cp.x) + " " + std::to_string(cp.y) + " " + std::to_string(cp.z);
			vertices.push_back(vertexStr);
		}
	}
	return vertices;
}

std::vector<std::string> FFS::generateObjFaces(const std::vector<std::vector<glm::vec3>>& controlPoints) {
	std::vector<std::string> faces;
	int numRows = controlPoints.size();
	int numCols = controlPoints[0].size();
	for (int i = 0; i < numRows - 1; i++) {
		for (int j = 0; j < numCols - 1; j++) {
			int v1 = i * numCols + j + 1;
			int v2 = i * numCols + j + 2;
			int v3 = (i + 1) * numCols + j + 1;

			std::string faceStr = "f " + std::to_string(v1) + " " + std::to_string(v2) + " " + std::to_string(v3);
			faces.push_back(faceStr);

			v1 = (i + 1) * numCols + j + 1;
			v2 = i * numCols + j + 2;
			v3 = (i + 1) * numCols + j + 2;

			faceStr = "f " + std::to_string(v1) + " " + std::to_string(v2) + " " + std::to_string(v3);
			faces.push_back(faceStr);
		}
	}
	return faces;
}

std::vector<std::string> FFS::getExportObjFormat() {
	std::vector<std::string> objs = this->generateObjVertices(this->generatedTerrain.Q);
	std::vector<std::string> faces = this->generateObjFaces(this->generatedTerrain.Q);
	std::vector<glm::vec2> surfaceUVs = this->generateTextureCoord(this->generatedTerrain.Q);
	std::vector<glm::vec3> surfaceNormals = this->generateNormals(this->generatedTerrain.Q);
	for (const glm::vec2& uv : surfaceUVs) {
		std::string uvStr = "vt " + std::to_string(uv.x) + " " + std::to_string(uv.y);
		objs.push_back(uvStr);
	}
	for (const glm::vec3& normal : surfaceNormals) {
		std::string normalStr = "vn " + std::to_string(normal.x) + " " + std::to_string(normal.y) + " " + std::to_string(normal.z);
		objs.push_back(normalStr);
	}
	objs.insert(objs.end(), faces.begin(), faces.end());
	return objs;
}

void FFS::detectControlPoints(const glm::vec3& mousePosition3D) {
	if (this->controlPoints.cpuGeom.verts.empty()) return;
	this->controlPointProperties.selectedControlPoints.clear();
	this->controlPointProperties.selectedWeights.clear();
	for (size_t i = 0; i < this->generatedTerrain.generatedPoints.size(); i++) {
		for (size_t j = 0; j < this->generatedTerrain.generatedPoints[i].size(); j++) {
			const glm::vec3* pointPos = &this->generatedTerrain.generatedPoints[i][j];
			float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos->x, pointPos->z));
			float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos->y));
			if (distanceXZ <= this->brushSettings.brushRadius && distanceY <= 1000.0f) {
				this->controlPointProperties.selectedControlPoints.push_back(&this->generatedTerrain.generatedPoints[i][j]);
				this->controlPointProperties.selectedWeights.push_back(&this->generatedTerrain.weights[i][j]);
			}
		}
	}
	this->controlPointsChangeColor(mousePosition3D, glm::vec3(0.0f, 0.0f, 1.0f));
}

void FFS::updateControlPoints(const glm::vec3& position) {
	if (this->controlPoints.cpuGeom.verts.empty() || this->controlPointProperties.selectedControlPoints.empty()) return;
	for (glm::vec3* point : this->controlPointProperties.selectedControlPoints) {
		*point += position;
	}
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::updateControlPoints(float weight) {
	if (this->controlPoints.cpuGeom.verts.empty() || this->controlPointProperties.selectedWeights.empty()) return;
	for (float* w : this->controlPointProperties.selectedWeights) {
		if (*w <= 1.0f && weight < 0.0f) {
			*w += weight / 20.0f;
			if (*w <= 0.01f) *w = 0.01f;
			continue;
		} else if (*w <= 1.0f && weight > 0.0f) {
			*w += weight / 20.0f;
			continue;
		}
		*w += weight;
	}
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

// MARK: - Colors

void FFS::controlPointsChangeColor(const glm::vec3& mousePosition3D, const glm::vec3& color) {
	for (size_t i = 0; i < this->controlPoints.cpuGeom.verts.size(); i++) {
		const glm::vec3* pointPos = &this->controlPoints.cpuGeom.verts[i];
		float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos->x, pointPos->z));
		float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos->y));
		this->controlPoints.cpuGeom.cols[i] = (distanceXZ <= this->brushSettings.brushRadius && distanceY <= 1000.0f) ? color : glm::vec3(1.0f, 0.0f, 0.0f);
	}
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);
}

// MARK: - Settings

// Terrain Settings

void FFS::createTerrain() {
	this->resetTerrain();
	this->generatedTerrain.generatedPoints = this->generateControlPoints();
	this->generatedTerrain.weights = this->generateWeights(this->generatedTerrain.generatedPoints.size(), this->generatedTerrain.generatedPoints[0].size());
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::resetTerrain() {
	this->controlPoints.cpuGeom.verts.clear(); this->controlPoints.cpuGeom.verts.shrink_to_fit(); std::vector<glm::vec3>().swap(this->controlPoints.cpuGeom.verts);
	this->controlPoints.cpuGeom.cols.clear(); this->controlPoints.cpuGeom.cols.shrink_to_fit(); std::vector<glm::vec3>().swap(this->controlPoints.cpuGeom.cols);
	this->freeFormSurface.cpuGeom.verts.clear(); this->freeFormSurface.cpuGeom.verts.shrink_to_fit(); std::vector<glm::vec3>().swap(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.cpuGeom.cols.clear(); this->freeFormSurface.cpuGeom.cols.shrink_to_fit(); std::vector<glm::vec3>().swap(this->freeFormSurface.cpuGeom.cols);
	this->generatedTerrain.generatedPoints.clear(); this->generatedTerrain.generatedPoints.shrink_to_fit(); std::vector<std::vector<glm::vec3>>().swap(this->generatedTerrain.generatedPoints);
	this->generatedTerrain.weights.clear(); this->generatedTerrain.weights.shrink_to_fit(); std::vector<std::vector<float>>().swap(this->generatedTerrain.weights);
	this->controlPointProperties.selectedControlPoints.clear(); this->controlPointProperties.selectedControlPoints.shrink_to_fit(); std::vector<glm::vec3*>().swap(this->controlPointProperties.selectedControlPoints);
	this->controlPointProperties.selectedWeights.clear(); this->controlPointProperties.selectedWeights.shrink_to_fit(); std::vector<float*>().swap(this->controlPointProperties.selectedWeights);
	this->controlPoints.gpuGeom.bind();
	this->controlPoints.gpuGeom.setVerts(this->controlPoints.cpuGeom.verts);
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);
	this->freeFormSurface.gpuGeom.bind();
	this->freeFormSurface.gpuGeom.setVerts(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.gpuGeom.setCols(this->freeFormSurface.cpuGeom.cols);
	this->nurbsSettings.k_u = 3;
	this->nurbsSettings.k_v = 3;
}

void FFS::resetTerrainToDefaults() {
	this->resetTerrain();
	this->terrainSettings.nControlPoints = 20;
	this->terrainSettings.terrainSize = 10.0f;
	this->terrainSettings.bIsChanging = true;
}

// NURBS Settings

void FFS::resetSelectedControlPoints() {
	if (this->controlPointProperties.selectedControlPoints.empty()) return;
	for (glm::vec3* point : this->controlPointProperties.selectedControlPoints) {
		*point = glm::vec3(point->x, 0.0f, point->z);
	}
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::resetSelectedWeights() {
	if (this->controlPointProperties.selectedWeights.empty()) return;
	for (float* w : this->controlPointProperties.selectedWeights) {
		*w = 1.0f;
	}
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::resetAllControlPoints() {
	this->generatedTerrain.generatedPoints = this->generateControlPoints();
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::resetAllWeights() {
	this->generatedTerrain.weights = this->generateWeights(this->generatedTerrain.generatedPoints.size(), this->generatedTerrain.generatedPoints.size());
	this->generateTerrain(this->generatedTerrain.generatedPoints, this->generatedTerrain.weights);
}

void FFS::resetNURBSToDefaults() {
	this->nurbsSettings.k_u = 3;
	this->nurbsSettings.k_v = 3;
	this->nurbsSettings.resolution = 100.0f;
	this->nurbsSettings.weightRate = 1.0f;
	this->nurbsSettings.bDisplayControlPoints = false;
	this->nurbsSettings.bDisplayLineSegments = false;
	this->nurbsSettings.bBezier = false;
	this->nurbsSettings.bClosedLoop = false;
	this->nurbsSettings.bIsChanging = true;
}

// Bursh Settings

void FFS::resetBurshToDefaults() {
	this->brushSettings.brushRadius = 1.5f;
	this->brushSettings.brushRateScale = 1.0f;
	this->brushSettings.bIsRising = true;
	this->brushSettings.bDisplayConvexHull = false;
	this->brushSettings.bIsPlanar = true;
}
