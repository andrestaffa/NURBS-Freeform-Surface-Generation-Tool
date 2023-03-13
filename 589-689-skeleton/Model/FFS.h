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

struct TerrainSettings {
	int nControlPoints = 20;
	float terrainSize = 10.0f;
	bool bIsChanging = false;
};

struct NURBSSettings {
	int k_u = 3;
	int k_v = 3;
	float resolution = 100.0f;
	float weightRate = 1.0f;
	bool bDisplayControlPoints = true;
	bool bDisplayLineSegments = true;
	bool bBezier = false;
	bool bClosedLoop = false;
	bool bIsChanging = false;
};

struct BrushSettings {
	float brushRadius = 1.5f;
	float brushRateScale = 1.0f;
	bool bIsRising = true;
	bool bDisplayConvexHull = false;
	bool bBrushingMode = true;
	bool bIsPlanar = true;
};

struct ControlPointProperties {
	std::vector<glm::vec3*> selectedControlPoints;
	std::vector<float*> selectedWeights;
};

class FFS {

private:

	// Processes
	Process controlPoints;
	Process freeFormSurface;
	Process nurbsLines;

	// Generated Control Points & Weights
	std::vector<std::vector<glm::vec3>> generatedPoints;
	std::vector<std::vector<float>> weights;

	// Control Point Properties
	ControlPointProperties controlPointProperties;

	// Terrain Settings
	TerrainSettings terrainSettings;
	// NURBS Settings
	NURBSSettings nurbsSettings;
	// Brush Settings
	BrushSettings brushSettings;
	
public:

	FFS();

public:

	void render();

private:

	// NURBS
	std::vector<float> generateKnotSequence(int length, int k);
	std::vector<std::vector<float>> generateWeights(int u_length, int v_length);
	int delta(const std::vector<float>& U, float u, int k, int m);
	glm::vec3 FFS_NURBS(const std::vector<std::vector<glm::vec3>>& P, const std::vector<float>& U, const std::vector<float>& V, std::vector<std::vector<float>> W, float u, float v, int k_u, int k_v, int m);

	// FFS
	std::vector<std::vector<glm::vec3>> generateControlPoints();
	void generateTerrain(const std::vector<std::vector<glm::vec3>>& P, const std::vector<std::vector<float>>& W);

	// Surface Properties
	std::vector<glm::vec3> generateQuads(const std::vector<std::vector<glm::vec3>>& points);
	std::vector<glm::vec2> generateQuads(const std::vector<std::vector<glm::vec2>>& points);
	std::vector<glm::vec2> generateTextureCoord(const std::vector<std::vector<glm::vec3>>& controlPoints);
	std::vector<glm::vec3> generateNormals(const std::vector<std::vector<glm::vec3>>& controlPoints);

	// .obj Formatting
	std::vector<std::string> generateOBJVertices(const std::vector<std::vector<glm::vec3>>& controlPoints);
	std::vector<std::string> generateOBJTextureCoord(const std::vector<std::vector<glm::vec3>>& controlPoints);
	std::vector<std::string> generateOBJNormals(const std::vector<std::vector<glm::vec3>>& controlPoints);
	std::vector<std::string> generateOBJFaces(const std::vector<std::vector<glm::vec3>>& controlPoints);

public:

	// Control Point Updates
	void detectControlPoints(const glm::vec3& mousePosition3D);
	void updateControlPoints(const glm::vec3& position);
	void updateControlPoints(float weight);

private:

	// Terrain Settings
	void controlPointsChangeColor(const glm::vec3& mousePosition3D, const glm::vec3& color);
	void resetTerrain();

public:

	// Processes
	const Process& getControlPoints() const { return this->controlPoints; }
	const Process& getFreeFormSurface() const { return this->freeFormSurface; }
	const Process& getNURBSControlPolygon() const { return this->nurbsLines; }

	// Generated Control Points & Weights
	const std::vector<std::vector<glm::vec3>>& getGeneratedControlPoints() const { return this->generatedPoints; }
	const std::vector<std::vector<float>>& getGeneratedWeights() const { return this->weights; }

	// Control Point Properties
	const ControlPointProperties& getControlPointProperties() const { return this->controlPointProperties; }

	// Terrain Settings
	void createTerrain();
	void resetTerrainToDefaults();
	TerrainSettings& getTerrainSettings() { return this->terrainSettings; }

	// NURBS Settings
	void resetSelectedControlPoints();
	void resetSelectedWeights();
	void resetAllControlPoints();
	void resetAllWeights();
	void resetNURBSToDefaults();
	NURBSSettings& getNURBSSettings() { return this->nurbsSettings; }

	// Brush Settings
	void resetBurshToDefaults();
	BrushSettings& getBrushSettings() { return this->brushSettings; }

};
