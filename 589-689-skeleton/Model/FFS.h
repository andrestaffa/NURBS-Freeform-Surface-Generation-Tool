#pragma once

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>
#include <unordered_map>

#include "../Geometry.h"
#include "../GLDebug.h"
#include "../Log.h"

struct Process { CPU_Geometry cpuGeom; GPU_Geometry gpuGeom; };

struct ControlPointProperties {
	std::vector<glm::vec3*> selectedControlPoints;
	std::vector<float*> selectedWeights;
};

class FFS {

private:

	Process controlPoints;
	Process freeFormSurface;

	std::vector<std::vector<glm::vec3>> generatedPoints;
	std::vector<std::vector<float>> weights;

	ControlPointProperties controlPointProperties;
	

public:
	FFS();

public:

	void render();

private:

	// NURBS
	std::vector<float> generateKnotSequence(int length, int k);
	std::vector<std::vector<float>> generateWeights(int u_length, int v_length);
	int delta(const std::vector<float>& U, float u, int k, int m);
	glm::vec3 FFS_NURBS(const std::vector<std::vector<glm::vec3>>& P, const std::vector<float>& U, const std::vector<float>& V, std::vector<std::vector<float>> W, float u, float v, int k, int m);

	// FFS
	std::vector<std::vector<glm::vec3>> generateControlPoints();
	void generateTerrain(const std::vector<std::vector<glm::vec3>>& P, const std::vector<std::vector<float>>& W);
	std::vector<glm::vec3> generateQuads(const std::vector<std::vector<glm::vec3>>& points);

public:

	// Ground Plane Detection of Control Points
	void detectControlPoints(const glm::vec3& mousePosition3D);

	void updateControlPoints(const glm::vec3& position);
	void updateControlPoints(float weight);


private:
	// Helpers
	void controlPointsChangeColor(const glm::vec3& mousePosition3D, const glm::vec3& color);

};
