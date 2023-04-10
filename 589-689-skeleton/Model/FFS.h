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

struct GeneratedTerrain {
	std::vector<std::vector<glm::vec3>> generatedPoints;
	std::vector<std::vector<float>> weights;
	std::vector<std::vector<glm::vec3>> Q;
};

struct ControlPointProperties {
	std::vector<glm::vec3*> selectedControlPoints;
	std::vector<float*> selectedWeights;
};

struct TerrainSettings {
	int nControlPoints = 55;
	float terrainSize = 10.0f;
	bool bIsChanging = false;
};

struct RandomGenerationSettings {
	float skipProbability = 0.70f;
	float minHeight = 0.0f;
	float maxHeight = 4.0f;
};

struct NURBSSettings {
	int k_u = 3;
	int k_v = 3;
	float resolution = 100.0f;
	float weightRate = 1.0f;
	bool bDisplayControlPoints = false;
	bool bDisplayLineSegments = false;
	bool bBezier = false;
	bool bIsChanging = false;
};

struct BrushSettings {
	float brushRadius = 1.5f;
	float brushRateScale = 1.0f;
	bool bIsRising = true;
	bool bDisplayBrushArea = true;
};

struct ImportNOBJSettings {
	std::vector<glm::vec3> controlPoints;
	std::vector<float> weights;
	int k_u = 3, k_v = 3;
	float resolution = 100.0f;
	int nControlPoints = 20;
	float terrainSize = 10.0f;
};

class FFS {

private:

	// Processes
	Process controlPoints;
	Process freeFormSurface;
	Process nurbsLines;
	Process selectedArea;

	// Generated Control Points, Weights & Curve
	GeneratedTerrain generatedTerrain;

	// Control Point Properties
	ControlPointProperties controlPointProperties;

	// Terrain Settings
	TerrainSettings terrainSettings;
	// Random Generation Settings
	RandomGenerationSettings randomGenerationSettings;
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
	std::vector<std::string> generateObjVertices(const std::vector<std::vector<glm::vec3>>& controlPoints);
	std::vector<std::string> generateObjFaces(const std::vector<std::vector<glm::vec3>>& controlPoints);

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

	// Generated Control Points, Weights & Curve
	const GeneratedTerrain& getGeneratedTerrain() const { return this->generatedTerrain; }

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

	// .obj Formatting
	std::vector<std::string> getExportObjFormat();

	// .nobj Formatting
	std::vector<std::string> getExportNObjFormat();
	bool getImportNObjFormat(const ImportNOBJSettings& settings);

	// Random Generation
	void generateRandomTerrain();
	void resetRandomGenerationSettings();
	RandomGenerationSettings& getRandomGenerationSettings() { return this->randomGenerationSettings; }


	// Reset All Settings
	void resetAllSettings();

};
