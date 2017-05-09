#pragma once
#ifndef INCLUDED_HEIGHTMAP_TERRAIN_H
#define INCLUDED_HEIGHTMAP_TERRAIN_H

#include <string>
#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include "PV112.h"

class Terrain : public PV112::Geometry {
public:
	std::vector<std::vector<float>> height;
};

//-----------------------------------------
//----            TERRAIN              ----
//-----------------------------------------

Terrain LoadHeightmapTerrain(const maybewchar* filename, GLint position_location, GLint normal_location, GLint tex_coord_location);

//-----------------------------------------
//----      Random trees planting      ----
//-----------------------------------------

void RandomTrees(const Terrain& terrain_geometry, glm::mat4 *tree_model_matrixes, int tree_count, std::function<float(float, float, float)> callable);

//-----------------------------------------
//----        BLINK CAMERA CLASS        ----
//-----------------------------------------

class BlinkCamera
{
private:
	/// Constants that defines the behaviour of the camera
	///		- Minimum elevation in radians
	static const float min_elevation;
	///		- Maximum elevation in radians
	static const float max_elevation;
	///		- Sensitivity of the mouse when changing elevation or direction angles
	static const float angle_sensitivity;
	///		- Camera move speed
	static const float move_speed;

	Terrain* terrain;

	float angle_direction;
	float angle_elevation;

	/// Final position of the eye in world space coordinates
	glm::vec3 eye_position;
	glm::vec3 look_position;

	/// Recomputes 'look_position' from 'angle_direction', 'angle_elevation', and 'distance'
	void update_look_pos();

	float get_height(float x, float z);

public:
	bool vel_w = false;
	bool vel_s = false;
	bool vel_a = false;
	bool vel_d = false;

	BlinkCamera() : terrain(nullptr), angle_direction(0.0f), angle_elevation(0.0f) { };
	
	BlinkCamera(Terrain* terrain, float x, float y);

	/// Call when the user presses or releases a mouse button (see glutMouseFunc)
	void OnMouseButtonChanged(int button, int state, int x, int y);

	void Move();

	/// Call when the user moves with the mouse cursor (see glutMotionFunc)
	void OnMouseMoved(int x, int y);

	/// Returns the position of the eye in world space coordinates
	glm::vec3 GetEyePosition() const;

	/// Returns the position for the eye to look at in world space coordinates
	glm::vec3 GetLookPosition() const;
};

#endif	// INCLUDED_HEIGHTMAP_TERRAIN_H