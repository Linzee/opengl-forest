// PV112 2017, Dominik Gmiterko, project

#include "PV112.h"
#include "HeightmapTerrain.h"

#include <iostream>
#include <random>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

// Include GLEW, use static library
#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment(lib, "glew32s.lib") // Link with GLEW library

// Include DevIL for image loading
#if defined(_WIN32)
#pragma comment(lib, "glew32s.lib")
// On Windows, we use Unicode dll of DevIL
// That also means we need to use wide strings
#ifndef _UNICODE
#define _UNICODE
#include <IL/il.h>
#undef _UNICODE
#else
#include <IL/il.h>
#endif
#pragma comment(lib, "DevIL.lib") // Link with DevIL library
typedef wchar_t maybewchar;
#define MAYBEWIDE(s) L##s
#else // On Linux, we need regular (not wide) strings
#include <IL/il.h>
typedef char maybewchar;
#define MAYBEWIDE(s) s
#endif

// Include FreeGLUT
#include <GL/freeglut.h>

// Include the most important GLM functions
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Current window size
int win_width = 1920;
int win_height = 1080;

// Buffer structures
static const int LIGHT_COUNT = 2;
struct Light
{
	glm::vec4 position;
	glm::vec4 ambient_color;
	glm::vec4 diffuse_color;
	glm::vec4 specular_color;
	glm::vec4 size;
};
struct Lights
{
	Light lights[LIGHT_COUNT];
};
Lights lights;
GLuint lights_ubo;

struct Camera
{
	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
	glm::vec3 eye_position;
};
Camera camera;
GLuint camera_ubo;

struct Material
{
	glm::vec4 ambient_color;
	glm::vec4 diffuse_color;
	glm::vec4 specular_color;
	float shininess;
};
Material material;
GLuint material_ubo;

static const int TREE_COUNT = 100;
static const int GRASS_COUNT = 1000;
struct TreeData
{
	glm::mat4 tree_model_matrix[GRASS_COUNT];
};
TreeData tree_data;
GLuint tree_data_ubo;
GLuint bush_data_ubo;
GLuint long_grass_data_ubo[12];

GLuint reflection_framebuffer;
GLuint reflection_tex;
GLuint reflection_depth;

// Terrain
GLuint terrain_program;

Terrain terrain_geometry;

GLuint terrain_grass_tex;
GLuint terrain_rocks_tex;
GLint terrain_grass_tex_loc;
GLint terrain_rocks_tex_loc;
GLint terrain_model_matrix_loc;

// Tree
GLuint tree_program;

PV112::Geometry tree_geometry;
PV112::Geometry bush_geometry;
PV112::Geometry long_grass_geometry[12];
PV112::Geometry lamp_geometry;

GLuint tree_tex;
GLuint bush_tex;
GLuint long_grass_tex;
GLint tree_tex_loc;
GLint tree_model_matrix_loc;
GLint tree_wind_height_loc;
GLint tree_app_time_loc;

// Water
GLuint water_program;

PV112::Geometry water_geometry;

GLuint water_normal_tex;
GLint water_normal_tex_loc;
GLint water_model_matrix_loc;
GLint water_app_time_loc;
GLint water_reflection_tex_loc;

// Simple camera that allows us to look at the object from different views
BlinkCamera my_camera;

// Current time of the application in seconds, for animations
float app_time = 0.0f;
float animation_speed = 0.020f;

// Called when the user presses a key
void key_down(unsigned char key, int mouseX, int mouseY)
{
	switch (key)
	{
	case 27: // Escape
		exit(0);
		break;
	case 'l':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;
	case 'f':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glutPostRedisplay();
		break;
	case 't':
		glutFullScreenToggle();
		break;

	case '+':
		animation_speed += 0.1;
		break;
	case '-':
		animation_speed -= 0.1;
		break;

	case 'w':
		my_camera.vel_w = true;
		break;
	case 's':
		my_camera.vel_s = true;
		break;
	case 'a':
		my_camera.vel_a = true;
		break;
	case 'd':
		my_camera.vel_d = true;
		break;
	}
}

void key_up(unsigned char key, int mouseX, int mouseY)
{
	switch (key)
	{
	case 'w':
		my_camera.vel_w = false;
		break;
	case 's':
		my_camera.vel_s = false;
		break;
	case 'a':
		my_camera.vel_a = false;
		break;
	case 'd':
		my_camera.vel_d = false;
		break;
	}
}


// Called when the user presses a mouse button
void mouse_button_changed(int button, int state, int x, int y)
{
	
}

// Called when the user moves with the mouse (when some mouse button is pressed)
void mouse_moved(int x, int y)
{
	static bool just_warped = false;

	if (just_warped) {
		just_warped = false;
		return;
	}

	int cx = win_width / 2;
	int cy = win_height / 2;
	my_camera.OnMouseMoved(x - cx, y - cy);
	
	glutWarpPointer(cx, cy);
	just_warped = true;
}

// Initializes OpenGL stuff
void init()
{
	glClearColor(0.33f, 0.38f, 0.45f, 1.0f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);

	int position_loc = 0;
	int normal_loc = 1;
	int tex_coord_loc = 2;

	// Create geometries
	terrain_geometry = LoadHeightmapTerrain(MAYBEWIDE("resources/heightmap.png"), position_loc, normal_loc, tex_coord_loc);
	tree_geometry = PV112::LoadOBJ("resources/tree1.obj", position_loc, normal_loc, tex_coord_loc);
	bush_geometry = PV112::LoadOBJ("resources/bush.obj", position_loc, normal_loc, tex_coord_loc);
	water_geometry = PV112::CreateGrid(200, position_loc, normal_loc, tex_coord_loc);
	for (int i = 0; i < 12; ++i) {
		std::ostringstream buffer;
		buffer << "resources/grass" << std::to_string(i + 1) << ".obj";
		long_grass_geometry[i] = PV112::LoadOBJ(buffer.str().c_str(), position_loc, normal_loc, tex_coord_loc);
	}
	lamp_geometry = PV112::LoadOBJ("resources/lamp.obj", position_loc, normal_loc, tex_coord_loc);

	my_camera = BlinkCamera(&terrain_geometry, 0.0f, 0.0f);

	// Create terrain program
	terrain_program = PV112::CreateAndLinkProgram("shaders/terrain_vertex.glsl", "shaders/terrain_fragment.glsl",
		position_loc, "position", normal_loc, "normal", tex_coord_loc, "tex_coord");
	if (0 == terrain_program)
		PV112::WaitForEnterAndExit();

	int terrain_light_loc = glGetUniformBlockIndex(terrain_program, "LightData");
	glUniformBlockBinding(terrain_program, terrain_light_loc, 0);

	int terrain_camera_loc = glGetUniformBlockIndex(terrain_program, "CameraData");
	glUniformBlockBinding(terrain_program, terrain_camera_loc, 1);

	int terrain_material_loc = glGetUniformBlockIndex(terrain_program, "MaterialData");
	glUniformBlockBinding(terrain_program, terrain_material_loc, 2);

	terrain_grass_tex_loc = glGetUniformLocation(terrain_program, "grass_tex");
	terrain_rocks_tex_loc = glGetUniformLocation(terrain_program, "rocks_tex");

	terrain_model_matrix_loc = glGetUniformLocation(terrain_program, "model_matrix");

	// Create tree program
	tree_program = PV112::CreateAndLinkProgram("shaders/tree_vertex.glsl", "shaders/tree_fragment.glsl",
		position_loc, "position", normal_loc, "normal", tex_coord_loc, "tex_coord");
	if (0 == tree_program)
		PV112::WaitForEnterAndExit();

	int tree_light_loc = glGetUniformBlockIndex(tree_program, "LightData");
	glUniformBlockBinding(tree_program, tree_light_loc, 0);

	int tree_camera_loc = glGetUniformBlockIndex(tree_program, "CameraData");
	glUniformBlockBinding(tree_program, tree_camera_loc, 1);

	int tree_material_loc = glGetUniformBlockIndex(tree_program, "MaterialData");
	glUniformBlockBinding(tree_program, tree_material_loc, 2);

	int tree_tree_data_loc = glGetUniformBlockIndex(tree_program, "TreeData");
	glUniformBlockBinding(tree_program, tree_tree_data_loc, 3);

	tree_tex_loc = glGetUniformLocation(tree_program, "tree_tex");

	tree_model_matrix_loc = glGetUniformLocation(tree_program, "model_matrix");

	tree_wind_height_loc = glGetUniformLocation(tree_program, "wind_height");

	tree_app_time_loc = glGetUniformLocation(tree_program, "app_time");

	// Create water program
	water_program = PV112::CreateAndLinkProgram("shaders/water_vertex.glsl", "shaders/water_fragment.glsl",
		position_loc, "position", normal_loc, "normal", tex_coord_loc, "tex_coord");
	if (0 == water_program)
		PV112::WaitForEnterAndExit();

	int water_light_loc = glGetUniformBlockIndex(water_program, "LightData");
	glUniformBlockBinding(water_program, water_light_loc, 0);

	int water_camera_loc = glGetUniformBlockIndex(water_program, "CameraData");
	glUniformBlockBinding(water_program, water_camera_loc, 1);

	int water_material_loc = glGetUniformBlockIndex(water_program, "MaterialData");
	glUniformBlockBinding(water_program, water_material_loc, 2);

	water_model_matrix_loc = glGetUniformLocation(water_program, "model_matrix");
	water_app_time_loc = glGetUniformLocation(water_program, "app_time");
	water_normal_tex_loc = glGetUniformLocation(water_program, "water_normal_tex");
	water_reflection_tex_loc = glGetUniformLocation(water_program, "reflection_tex");

	// Light
	for (int i = 0; i < LIGHT_COUNT; ++i) {
		lights.lights[i].position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0);
		lights.lights[i].ambient_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		lights.lights[i].diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lights.lights[i].specular_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	glGenBuffers(1, &lights_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Lights), &lights, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Camera
	camera.view_matrix = glm::mat4(1.0f);
	camera.projection_matrix = glm::mat4(1.0f);
	camera.eye_position = glm::vec3(0.0f);

	glGenBuffers(1, &camera_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera), &camera, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Material
	material.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);
	material.shininess = 1.0f;

	glGenBuffers(1, &material_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), &material, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Tree locations buffer
	RandomTrees(terrain_geometry, tree_data.tree_model_matrix, TREE_COUNT, [](float x, float y, float z) { return 1.0; });
	glGenBuffers(1, &tree_data_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, tree_data_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(tree_data), &tree_data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	RandomTrees(terrain_geometry, tree_data.tree_model_matrix, TREE_COUNT, [](float x, float y, float z) { return y < 0.5 ? 0.1f : 1.0; });
	glGenBuffers(1, &bush_data_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, bush_data_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(tree_data), &tree_data, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	for (int i = 0; i < 12; ++i) {
		RandomTrees(terrain_geometry, tree_data.tree_model_matrix, GRASS_COUNT, [](float x, float y, float z) { return y < 0.1 ? 0.0 : 1 - y; });
		glGenBuffers(1, &long_grass_data_ubo[i]);
		glBindBuffer(GL_UNIFORM_BUFFER, long_grass_data_ubo[i]);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(tree_data), &tree_data, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	// Grass texture
	terrain_grass_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/grass.png"));
	glBindTexture(GL_TEXTURE_2D, terrain_grass_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Rocks texture
	terrain_rocks_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/rocks.png"));
	glBindTexture(GL_TEXTURE_2D, terrain_rocks_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Tree texture
	tree_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/tree1.png"));
	glBindTexture(GL_TEXTURE_2D, tree_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bush texture
	bush_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/bush.tga"));
	glBindTexture(GL_TEXTURE_2D, bush_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Long grass texture
	long_grass_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/long_grass.tga"));
	glBindTexture(GL_TEXTURE_2D, long_grass_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Water normal texture
	water_normal_tex = PV112::CreateAndLoadTexture(MAYBEWIDE("resources/water_normal.png"));
	glBindTexture(GL_TEXTURE_2D, water_normal_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Reflection texture
	glGenFramebuffers(1, &reflection_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, reflection_framebuffer);

	glGenTextures(1, &reflection_tex);
	glBindTexture(GL_TEXTURE_2D, reflection_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_width, win_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_MIRRORED_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	
	glGenRenderbuffers(1, &reflection_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, reflection_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, win_width, win_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflection_depth);
	
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, reflection_tex, 0);
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		PV112::WaitForEnterAndExit();


	// Skybox cube texture
	/*
	skybox_tex = CreateAndLoadTextureCube(MAYBEWIDE("skybox_px.png"),
	MAYBEWIDE("skybox_nx.png"), MAYBEWIDE("skybox_py.png"),
	MAYBEWIDE("skybox_ny.png"), MAYBEWIDE("skybox_pz.png"),
	MAYBEWIDE("skybox_nz.png"));
	*/
}

//Forward-declaration of functions
void renderTerrain();
void renderLamp();
void renderTrees();
void renderWater();

// Called when the window needs to be rerendered
void render()
{
	float day_time = 1 - pow(sin(app_time / 120.0f), 4.0f);

	glClearColor(0.66f * day_time, 0.76f * day_time, 0.90f * day_time, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lights_ubo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, camera_ubo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, material_ubo);
	
	// Light position, with a simple animation
	lights.lights[0].position =
		glm::rotate(glm::mat4(1.0f), app_time * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, 20.0f, 0.0f)) *
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	lights.lights[0].diffuse_color = glm::vec4(1.2f, 1.2f, 1.2f, 1.0f) * day_time;
	lights.lights[0].ambient_color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f) * day_time;
	lights.lights[0].size = glm::vec4(10000.0f, 10000.0f, 10000.0f, 1.0f);
	
	lights.lights[1].position = glm::vec4(0.0f, terrain_geometry.height[128][128] * 32.0f - 2.0f + 5.6f, 0.0f, 1.0f);
	lights.lights[1].diffuse_color = glm::vec4(3 * 1.00f, 3 * 0.98f, 3 * 0.56f, 1.0f) * (1 - day_time);
	lights.lights[1].ambient_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) * (1 - day_time);
	lights.lights[1].size = glm::vec4(20.0f, 20.0f, 20.0f, 1.0f);
	
	glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Lights), &lights);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	/*
		Reflection rendering
	*/

	glBindFramebuffer(GL_FRAMEBUFFER, reflection_framebuffer);
	glViewport(0, 0, win_width, win_height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CLIP_DISTANCE0);

	// Camera matrices and eye position
	camera.projection_matrix = glm::perspective(glm::radians(45.0f), float(win_width) / float(win_height), 0.1f, 1000.0f);
	camera.view_matrix = glm::lookAt(my_camera.GetEyePosition(), my_camera.GetLookPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.eye_position = my_camera.GetEyePosition();

	camera.view_matrix = glm::scale(camera.view_matrix, glm::vec3(1.0, -1.0, 1.0));

	glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Camera), &camera);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Geometries
	renderTerrain();
	renderLamp();
	renderTrees();
	
	glDisable(GL_CLIP_DISTANCE0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, win_width, win_height);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
		Main rendering
	*/

	// Camera matrices and eye position
	camera.projection_matrix = glm::perspective(glm::radians(45.0f), float(win_width) / float(win_height), 0.1f, 1000.0f);
	camera.view_matrix = glm::lookAt(my_camera.GetEyePosition(), my_camera.GetLookPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.eye_position = my_camera.GetEyePosition();

	glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Camera), &camera);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Geometries
	renderTerrain();
	renderLamp();
	renderTrees();
	renderWater();
	
	glBindVertexArray(0);
	glUseProgram(0);

	glutSwapBuffers();
}

void renderTerrain() {
	
	glUseProgram(terrain_program);

	glBindVertexArray(terrain_geometry.VAO);

	material.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);
	material.shininess = 1.0f;

	glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Material), &material);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glm::mat4 model_matrix(1.0f);
	model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, -2.0f, 0.0f));
	model_matrix = glm::scale(model_matrix, glm::vec3(100.0f, 32.0f, 100.0f));
	glUniformMatrix4fv(terrain_model_matrix_loc, 1, GL_FALSE, glm::value_ptr(model_matrix));

	glUniform1i(terrain_grass_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrain_grass_tex);

	glUniform1i(terrain_rocks_tex_loc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain_rocks_tex);

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(4294967295U);
	PV112::DrawGeometry(terrain_geometry);
	glDisable(GL_PRIMITIVE_RESTART);
}

void renderLamp() {

	glUseProgram(terrain_program);

	glBindVertexArray(lamp_geometry.VAO);

	material.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular_color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
	material.shininess = 6.0f;

	glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Material), &material);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glm::mat4 model_matrix(1.0f);
	model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, terrain_geometry.height[128][128] * 32.0f - 2.0f, 0.0f));
	model_matrix = glm::scale(model_matrix, glm::vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(terrain_model_matrix_loc, 1, GL_FALSE, glm::value_ptr(model_matrix));

	glUniform1i(terrain_grass_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrain_grass_tex);

	glUniform1i(terrain_rocks_tex_loc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, terrain_rocks_tex);

	PV112::DrawGeometry(lamp_geometry);
}


void renderTrees() {
	glUseProgram(tree_program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1f(tree_app_time_loc, app_time);

	material.ambient_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);
	material.shininess = 1.0f;

	glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Material), &material);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	glm::mat4 model_matrix(1.0f);
	model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, -2.0f, 0.0f));
	model_matrix = glm::scale(model_matrix, glm::vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(tree_model_matrix_loc, 1, GL_FALSE, glm::value_ptr(model_matrix));

	glBindVertexArray(tree_geometry.VAO);

	glUniform1f(tree_wind_height_loc, 15.0);

	glUniform1i(tree_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tree_tex);

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, tree_data_ubo);

	PV112::DrawGeometryInstanced(tree_geometry, TREE_COUNT);
	
	
	glBindVertexArray(bush_geometry.VAO);

	glUniform1f(tree_wind_height_loc, 10.0);

	glUniform1i(tree_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bush_tex);

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, bush_data_ubo);

	PV112::DrawGeometryInstanced(bush_geometry, TREE_COUNT);


	glUniform1f(tree_wind_height_loc, 2.0);

	glUniform1i(tree_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, long_grass_tex);

	for (int i = 0; i < 12; ++i) {
		glBindVertexArray(long_grass_geometry[i].VAO);
		glBindBufferBase(GL_UNIFORM_BUFFER, 3, long_grass_data_ubo[i]);
		PV112::DrawGeometryInstanced(long_grass_geometry[i], GRASS_COUNT);
	}

	glDisable(GL_BLEND);
}

void renderWater() {
	glm::mat4 model_matrix;

	glUseProgram(water_program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(water_geometry.VAO);

	material.ambient_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	material.diffuse_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	material.specular_color = glm::vec4(8.0f, 8.0f, 8.0f, 1.0f);
	material.shininess = 200.0f;

	glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Material), &material);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 0.2f, 100.0f));
	glUniformMatrix4fv(water_model_matrix_loc, 1, GL_FALSE, glm::value_ptr(model_matrix));

	glUniform1f(water_app_time_loc, app_time * 0.05f);

	glUniform1i(water_normal_tex_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, water_normal_tex);

	glUniform1i(water_reflection_tex_loc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, reflection_tex);

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(4294967295U);
	PV112::DrawGeometry(water_geometry);
	glDisable(GL_PRIMITIVE_RESTART);

	glDisable(GL_BLEND);
}

// Called when the window changes its size
void reshape(int width, int height)
{
	win_width = width;
	win_height = height;

	// Set the area into which we render
	glViewport(0, 0, win_width, win_height);
}

// Callback function to be called when we make an error in OpenGL
void GLAPIENTRY simple_debug_callback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << message << std::endl; // Put the breakpoint here
		return;
	default:
		return;
	}
}

// Simple timer function for animations
void timer(int)
{
	app_time += animation_speed;
	my_camera.Move();
	glutTimerFunc(20, timer, 0);
	glutPostRedisplay();
}

int main(int argc, char ** argv)
{
	//std::string line;
	//std::cin >> line;

	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// Set OpenGL Context parameters
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	// Initialize window
	glutInitWindowSize(win_width, win_height);
	glutCreateWindow("Dominik Gmiterko, project PV112");

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	glutFullScreenToggle();

	// Initialize DevIL library
	ilInit();

	// Set the debug callback
	PV112::SetDebugCallback(simple_debug_callback);

	// Initialize our OpenGL stuff
	init();

	// Register callbacks
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key_down);
	glutKeyboardUpFunc(key_up);
	glutTimerFunc(20, timer, 0);
	glutMouseFunc(mouse_button_changed);
	glutMotionFunc(mouse_moved);
	glutPassiveMotionFunc(mouse_moved);

	glutSetCursor(GLUT_CURSOR_NONE);

	// Run the main loop
	glutMainLoop();

	return 0;
}
