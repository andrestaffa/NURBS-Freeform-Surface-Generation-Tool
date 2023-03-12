#include "FFS.h"

FFS::FFS() {
	this->generatedPoints = this->generateControlPoints();
	this->weights = this->generateWeights(this->generatedPoints.size(), this->generatedPoints.size());
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::render() {
	if (this->terrainSettings.bIsChanging) this->createTerrain();
	if (this->nurbsSettings.bIsChanging) this->generateTerrain(this->generatedPoints, this->weights);
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
	for (int i = order; i < numKnots - order; i++) {
		U[i] = (float)(i - order + 1) / (float)(numControlPoints - order + 1);
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
	std::vector<std::vector<glm::vec3>> Q;

	std::vector<float> U = this->generateKnotSequence(P.size(), this->nurbsSettings.k_u);
	std::vector<float> V = this->generateKnotSequence(P[0].size(), this->nurbsSettings.k_v);

	int i = 0;
	for (float u = U[this->nurbsSettings.k_u - 1]; u <= U[P.size() + 1]; u += (1.0f / this->nurbsSettings.resolution)) {
		Q.push_back(std::vector<glm::vec3>());
		for (float v = V[this->nurbsSettings.k_v - 1]; v <= V[P[0].size() + 1]; v += (1.0f / this->nurbsSettings.resolution)) {
			Q[i].push_back(this->FFS_NURBS(P, U, V, W, u, v, this->nurbsSettings.k_u, this->nurbsSettings.k_v, P.size()));
		}
		i += 1;
	} 

	std::vector<glm::vec3> surface = this->generateQuads(Q);
	std::vector<glm::vec3> quadPoints = this->generateQuads(P);

	// Surface
	this->freeFormSurface.gpuGeom.bind();
	this->freeFormSurface.cpuGeom.verts = surface;
	this->freeFormSurface.cpuGeom.cols.resize(this->freeFormSurface.cpuGeom.verts.size(), this->terrainSettings.terrainColor);
	this->freeFormSurface.gpuGeom.setVerts(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.gpuGeom.setCols(this->freeFormSurface.cpuGeom.cols);

	// Control Points
	this->controlPoints.gpuGeom.bind();
	this->controlPoints.cpuGeom.verts = quadPoints;
	this->controlPoints.cpuGeom.cols.resize(this->controlPoints.cpuGeom.verts.size(), this->terrainSettings.primaryColor);
	this->controlPoints.gpuGeom.setVerts(this->controlPoints.cpuGeom.verts);
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);

	// NURBS Lines
	this->nurbsLines.gpuGeom.bind();
	this->nurbsLines.cpuGeom.verts = quadPoints;
	this->nurbsLines.cpuGeom.cols.resize(this->nurbsLines.cpuGeom.verts.size(), this->terrainSettings.nurbsLineColor);
	this->nurbsLines.gpuGeom.setVerts(this->nurbsLines.cpuGeom.verts);
	this->nurbsLines.gpuGeom.setCols(this->nurbsLines.cpuGeom.cols);

}

std::vector<glm::vec3> FFS::generateQuads(const std::vector<std::vector<glm::vec3>>& points) {
	std::vector<glm::vec3> R;
	for (size_t i = 0; i < points.size() - 1; i++) {
		for (size_t j = 0; j < points[i].size() - 1; j++) {
			R.push_back(points[i][j + 1]); // Top Left
			R.push_back(points[i][j]); // Bottom Left
			R.push_back(points[i + 1][j]); // Bottom Right
			R.push_back(points[i][j + 1]); // Top Left
			R.push_back(points[i + 1][j + 1]); // Top Right
			R.push_back(points[i + 1][j]); // Bottom Right
		}
	}
	return R;
}

// Ground Plane Detection of Control Points (NEED MAKE MUCH MORE EFFICIENT ALGORITHM)

void FFS::detectControlPoints(const glm::vec3& mousePosition3D) {
	if (this->controlPoints.cpuGeom.verts.empty()) return;
	this->controlPointProperties.selectedControlPoints.clear();
	this->controlPointProperties.selectedWeights.clear();
	for (size_t i = 0; i < this->generatedPoints.size(); i++) {
		for (size_t j = 0; j < this->generatedPoints[i].size(); j++) {
			const glm::vec3* pointPos = &this->generatedPoints[i][j];
			float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos->x, pointPos->z));
			float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos->y));
			if (distanceXZ <= this->brushSettings.brushRadius && distanceY <= 1000.0f) {
				this->controlPointProperties.selectedControlPoints.push_back(&this->generatedPoints[i][j]);
				this->controlPointProperties.selectedWeights.push_back(&this->weights[i][j]);
			}
		}
	}
	this->controlPointsChangeColor(mousePosition3D, this->terrainSettings.selectedColor);
	this->terrainChangeColor(this->terrainSettings.terrainColor);
	this->nurbsChangeLineColor(this->terrainSettings.nurbsLineColor);
}

void FFS::updateControlPoints(const glm::vec3& position) {
	if (this->controlPoints.cpuGeom.verts.empty() || this->controlPointProperties.selectedControlPoints.empty()) return;
	for (glm::vec3* point : this->controlPointProperties.selectedControlPoints) {
		*point += position;
	}
	this->generateTerrain(this->generatedPoints, this->weights);
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
	this->generateTerrain(this->generatedPoints, this->weights);
}

// MARK: - Colors

void FFS::controlPointsChangeColor(const glm::vec3& mousePosition3D, const glm::vec3& color) {
	for (size_t i = 0; i < this->controlPoints.cpuGeom.verts.size(); i++) {
		const glm::vec3* pointPos = &this->controlPoints.cpuGeom.verts[i];
		float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos->x, pointPos->z));
		float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos->y));
		this->controlPoints.cpuGeom.cols[i] = (distanceXZ <= this->brushSettings.brushRadius && distanceY <= 1000.0f) ? color : this->terrainSettings.primaryColor;
	}
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);
}

void FFS::terrainChangeColor(const glm::vec3& color) {
	for (glm::vec3& col : this->freeFormSurface.cpuGeom.cols) col = color;
	this->freeFormSurface.gpuGeom.setCols(this->freeFormSurface.cpuGeom.cols);
}

void FFS::nurbsChangeLineColor(const glm::vec3& color) {
	for (glm::vec3& col : this->nurbsLines.cpuGeom.cols) col = color;
	this->nurbsLines.gpuGeom.setCols(this->nurbsLines.cpuGeom.cols);
}

// MARK: - Settings

// Terrain Settings

void FFS::createTerrain() {
	this->resetTerrain();
	this->generatedPoints = this->generateControlPoints();
	this->weights = this->generateWeights(this->generatedPoints.size(), this->generatedPoints.size());
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::resetTerrain() {
	this->controlPoints.cpuGeom.verts.clear(); this->controlPoints.cpuGeom.verts.shrink_to_fit(); std::vector<glm::vec3>().swap(this->controlPoints.cpuGeom.verts);
	this->controlPoints.cpuGeom.cols.clear(); this->controlPoints.cpuGeom.cols.shrink_to_fit(); std::vector<glm::vec3>().swap(this->controlPoints.cpuGeom.cols);
	this->freeFormSurface.cpuGeom.verts.clear(); this->freeFormSurface.cpuGeom.verts.shrink_to_fit(); std::vector<glm::vec3>().swap(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.cpuGeom.cols.clear(); this->freeFormSurface.cpuGeom.cols.shrink_to_fit(); std::vector<glm::vec3>().swap(this->freeFormSurface.cpuGeom.cols);
	this->generatedPoints.clear(); this->generatedPoints.shrink_to_fit(); std::vector<std::vector<glm::vec3>>().swap(this->generatedPoints);
	this->weights.clear(); this->weights.shrink_to_fit(); std::vector<std::vector<float>>().swap(this->weights);
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
	this->terrainSettings.terrainColor = glm::vec3(0.0f, 0.0f, 0.0f);
	this->terrainSettings.primaryColor = glm::vec3(1.0f, 0.0f, 0.0f);
	this->terrainSettings.selectedColor = glm::vec3(0.0f, 0.0f, 1.0f);
	this->terrainSettings.nurbsLineColor = glm::vec3(0.0f, 1.0f, 0.0f);
	this->terrainSettings.terrainSize = 10.0f;
	this->terrainSettings.bIsChanging = true;
}

// NURBS Settings

void FFS::resetSelectedControlPoints() {
	if (this->controlPointProperties.selectedControlPoints.empty()) return;
	for (glm::vec3* point : this->controlPointProperties.selectedControlPoints) {
		*point = glm::vec3(point->x, 0.0f, point->z);
	}
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::resetSelectedWeights() {
	if (this->controlPointProperties.selectedWeights.empty()) return;
	for (float* w : this->controlPointProperties.selectedWeights) {
		*w = 1.0f;
	}
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::resetAllControlPoints() {
	this->generatedPoints = this->generateControlPoints();
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::resetAllWeights() {
	this->weights = this->generateWeights(this->generatedPoints.size(), this->generatedPoints.size());
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::resetNURBSToDefaults() {
	this->nurbsSettings.k_u = 3;
	this->nurbsSettings.k_v = 3;
	this->nurbsSettings.resolution = 100.0f;
	this->nurbsSettings.weightRate = 1.0f;
	this->nurbsSettings.bDisplayControlPoints = true;
	this->nurbsSettings.bDisplayLineSegments = true;
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
	this->brushSettings.bBrushingMode = true;
	this->brushSettings.bIsPlanar = true;
}
