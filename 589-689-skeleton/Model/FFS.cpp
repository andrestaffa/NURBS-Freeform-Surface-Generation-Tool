#include "FFS.h"

FFS::FFS() {
	this->generatedPoints = this->generateControlPoints();
	this->weights = this->generateWeights(this->generatedPoints.size(), this->generatedPoints.size());
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::render() {
	glPointSize(10.0f);
	this->controlPoints.gpuGeom.bind();
	glDrawArrays(GL_POINTS, 0, GLsizei(this->controlPoints.cpuGeom.verts.size()));
	glLineWidth(10.0f);
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

glm::vec3 FFS::FFS_NURBS(const std::vector<std::vector<glm::vec3>>& P, const std::vector<float>& U, const std::vector<float>& V, std::vector<std::vector<float>> W, float u, float v, int k, int m) {
	std::vector<glm::vec3> D(k), C(k);
	std::vector<float> NV(k);
	int d = delta(U, u, k, m);
	int d_prime = delta(V, v, k, m);
	for (int i = 0; i <= k - 1; i++) {
		for (int j = 0; j <= k - 1; j++) {
			D[j] = P[d - i][d_prime - j] * W[d - i][d_prime - j];
			NV[j] = W[d - i][d_prime - j];
		}
		for (int r = k; r >= 2; r--) {
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
	for (int r = k; r >= 2; r--) {
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
	std::vector<std::vector<glm::vec3>> P(20, std::vector<glm::vec3>(20));

	float x_min = -10.0f;
	float x_max = 10.0f;
	float z_min = -10.0f;
	float z_max = 10.0f;

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

	int k = 3;

	std::vector<float> U = this->generateKnotSequence(P.size(), k);
	std::vector<float> V = this->generateKnotSequence(P[0].size(), k);

	int i = 0;
	for (float u = U[k - 1]; u <= U[P.size() + 1]; u += 0.01f) {
		Q.push_back(std::vector<glm::vec3>());
		for (float v = V[k - 1]; v <= V[P[0].size() + 1]; v += 0.01f) {
			Q[i].push_back(this->FFS_NURBS(P, U, V, W, u, v, k, P.size()));
		}
		i += 1;
	} 

	std::vector<glm::vec3> surface = this->generateQuads(Q);
	std::vector<glm::vec3> quadPoints = this->generateQuads(P);

	// Surface
	this->freeFormSurface.gpuGeom.bind();
	this->freeFormSurface.cpuGeom.verts = surface;
	this->freeFormSurface.cpuGeom.cols.resize(this->freeFormSurface.cpuGeom.verts.size(), glm::vec3(0.0f));
	this->freeFormSurface.gpuGeom.setVerts(this->freeFormSurface.cpuGeom.verts);
	this->freeFormSurface.gpuGeom.setCols(this->freeFormSurface.cpuGeom.cols);

	// Control Points
	this->controlPoints.gpuGeom.bind();
	this->controlPoints.cpuGeom.verts = quadPoints;
	this->controlPoints.cpuGeom.cols.resize(this->controlPoints.cpuGeom.verts.size(), glm::vec3(1.0f, 0.0f, 0.0f));
	this->controlPoints.gpuGeom.setVerts(this->controlPoints.cpuGeom.verts);
	this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);

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

// Ground Plane Detection of Control Points

void FFS::detectControlPoints(const glm::vec3& mousePosition3D) {
	this->controlPointProperties.selectedControlPoints.clear();
	this->controlPointProperties.selectedWeights.clear();
	for (size_t i = 0; i < this->generatedPoints.size(); i++) {
		for (size_t j = 0; j < this->generatedPoints[i].size(); j++) {
			glm::vec3& pointPos = this->generatedPoints[i][j];
			float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos.x, pointPos.z));
			float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos.y));
			if (distanceXZ <= 1.5f && distanceY <= 1000.0f) {
				this->controlPointProperties.selectedControlPoints.push_back(&this->generatedPoints[i][j]);
				this->controlPointProperties.selectedWeights.push_back(&this->weights[i][j]);
			}
		}
	}
	this->controlPointsChangeColor(mousePosition3D, glm::vec3(0.0f, 0.0f, 1.0f));
}

void FFS::updateControlPoints(const glm::vec3& position) {
	if (this->controlPointProperties.selectedControlPoints.empty()) return;
	for (glm::vec3* point : this->controlPointProperties.selectedControlPoints) {
		*point += position;
	}
	this->generateTerrain(this->generatedPoints, this->weights);
}

void FFS::updateControlPoints(float weight) {
	if (this->controlPointProperties.selectedWeights.empty()) return;
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

// MARK: - Helpers

void FFS::controlPointsChangeColor(const glm::vec3& mousePosition3D, const glm::vec3& color) {
	for (size_t i = 0; i < this->controlPoints.cpuGeom.verts.size(); i++) {
		glm::vec3& pointPos = this->controlPoints.cpuGeom.verts[i];
		float distanceXZ = glm::length(glm::vec2(mousePosition3D.x, mousePosition3D.z) - glm::vec2(pointPos.x, pointPos.z));
		float distanceY = glm::length(glm::vec1(mousePosition3D.y) - glm::vec1(pointPos.y));
		if (distanceXZ <= 1.5f && distanceY <= 1000.0f) {
			this->controlPoints.cpuGeom.cols[i] = color;
			this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);
		} else {
			this->controlPoints.cpuGeom.cols[i] = glm::vec3(1.0f, 0.0f, 0.0f);
			this->controlPoints.gpuGeom.setCols(this->controlPoints.cpuGeom.cols);
		}
	}
}
