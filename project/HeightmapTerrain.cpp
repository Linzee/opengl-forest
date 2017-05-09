#include "HeightmapTerrain.h"

//-----------------------------------------
//----            TERRAIN              ----
//-----------------------------------------

Terrain LoadHeightmapTerrain(const maybewchar* filename, GLint position_location, GLint normal_location, GLint tex_coord_location) {
	/*
		Load texture data
	*/
	Terrain terrain;

	// Create IL image
	ILuint IL_tex;
	ilGenImages(1, &IL_tex);

	ilBindImage(IL_tex);

	// Solve upside down textures
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

	// Load IL image
	ILboolean success = ilLoadImage(filename);
	if (!success)
	{
		ilBindImage(0);
		ilDeleteImages(1, &IL_tex);
		throw std::invalid_argument("Cannot load heightmap!");
	}

	// Get IL image parameters
	int img_width = ilGetInteger(IL_IMAGE_WIDTH);
	int img_height = ilGetInteger(IL_IMAGE_HEIGHT);
	int img_format = ilGetInteger(IL_IMAGE_FORMAT);
	int img_type = ilGetInteger(IL_IMAGE_TYPE);

	/*
		Load vectors
	*/
	std::vector< std::vector< glm::vec3> > vertexes(img_width, std::vector<glm::vec3>(img_height));
	std::vector< std::vector< glm::vec2> > coords(img_width, std::vector<glm::vec2>(img_height));

	terrain.height = std::vector< std::vector<float> >(img_width, std::vector<float>(img_height));

	ILubyte * imageData = ilGetData();

	for (int x = 0; x < img_width; x++) {
		for (int y = 0; y < img_height; y++) {
			float s = float(x) / float(img_width);
			float t = float(y) / float(img_height);
			
			int bl;
			switch (img_format)
			{
			case IL_RGB:  bl = 3;  break;
			case IL_RGBA: bl = 4; break;
			default:
				// Unsupported format
				ilBindImage(0);
				ilDeleteImages(1, &IL_tex);
				throw std::invalid_argument("Cannot load heightmap, invalid format!");
			}
			
			float height = float(imageData[(x*img_width + y) * bl]) / 255.0f;
			
			vertexes[x][y] = glm::vec3(-0.5f + s, height, -0.5f + t);
			coords[x][y] = glm::vec2(s, t);
			terrain.height[x][y] = height;
		}
	}

	ilBindImage(0);
	ilDeleteImages(1, &IL_tex);

	/*
		Calculate normals
	*/
	std::vector< std::vector<glm::vec3> > normals[2];
	for (int i = 0; i < 2; i++)
	{
		normals[i] = std::vector< std::vector<glm::vec3> >(img_width - 1, std::vector<glm::vec3>(img_height - 1));
	}

	for (int x = 0; x < img_width - 1; x++) {
		for (int y = 0; y < img_height - 1; y++) {
			glm::vec3 triangle0[] =
			{
				vertexes[x + 1][y + 1],
				vertexes[x + 1][y],
				vertexes[x][y]
			};
			glm::vec3 triangle1[] =
			{
				vertexes[x][y],
				vertexes[x][y + 1],
				vertexes[x + 1][y + 1],
			};

			glm::vec3 triangleNorm0 = glm::cross(triangle0[0] - triangle0[1], triangle0[1] - triangle0[2]);
			glm::vec3 triangleNorm1 = glm::cross(triangle1[0] - triangle1[1], triangle1[1] - triangle1[2]);

			normals[0][x][y] = glm::normalize(triangleNorm0);
			normals[1][x][y] = glm::normalize(triangleNorm1);
		}
	}

	std::vector< std::vector<glm::vec3> > finalNormals(img_width, std::vector<glm::vec3>(img_height));

	for (int x = 0; x < img_width; x++) {
		for (int y = 0; y < img_height; y++) {
			
			glm::vec3 finalNormal = glm::vec3(0.0f, 0.0f, 0.0f);

			// Look for bottom-right triangles
			if (x < img_width - 1 && y < img_height - 1) {
				finalNormal += normals[0][x][y];
				finalNormal += normals[1][x][y];
			}
			// Look for upper-left triangles
			if (x - 1 >= 0 && y - 1 >= 0) {
				finalNormal += normals[0][x - 1][y - 1];
				finalNormal += normals[1][x - 1][y - 1];
			}
			// Look for upper-right triangles
			if (x < img_width - 1 && y - 1 >= 0) {
				finalNormal += normals[0][x][y - 1];
			}
			// Look for bottom-left triangles
			if (x - 1 >= 0 && y < img_height - 1) {
				finalNormal += normals[1][x - 1][y];
			}
			
			finalNormals[x][y] = glm::normalize(finalNormal);
		}
	}

	/*
		Indices
	*/
	std::vector<unsigned int> indices;
	for (int y = 0; y < img_height - 1; y++) {
		for (int x = 0; x < img_width - 1; x++) {
			for (int r = 0; r < 2; r++) {
				int row = y + (1 - r);
				int index = row * img_width + x;
				indices.push_back(index);
			}
		}
		// Restart triangle strips
		indices.push_back(4294967295U);
	}

	/*
		Normalize data
	*/
	std::vector<float> vertexData(img_width * img_height * 8);
	for (int x = 0; x < img_width; x++) {
		for (int y = 0; y < img_height; y++) {
			vertexData[(x + y * img_width) * 8 + 0] = vertexes[x][y].x;
			vertexData[(x + y * img_width) * 8 + 1] = vertexes[x][y].y;
			vertexData[(x + y * img_width) * 8 + 2] = vertexes[x][y].z;

			vertexData[(x + y * img_width) * 8 + 3] = finalNormals[x][y].x;
			vertexData[(x + y * img_width) * 8 + 4] = finalNormals[x][y].y;
			vertexData[(x + y * img_width) * 8 + 5] = finalNormals[x][y].z;

			vertexData[(x + y * img_width) * 8 + 6] = coords[x][y].x;
			vertexData[(x + y * img_width) * 8 + 7] = coords[x][y].y;
		}
	}

	/*
		Load to opengl
	*/

	// Create a single buffer for vertex data
	glGenBuffers(1, &terrain.VertexBuffers[0]);
	glBindBuffer(GL_ARRAY_BUFFER, terrain.VertexBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create a buffer for indices
	glGenBuffers(1, &terrain.IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create a vertex array object for the geometry
	glGenVertexArrays(1, &terrain.VAO);

	// Set the parameters of the geometry
	glBindVertexArray(terrain.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, terrain.VertexBuffers[0]);
	if (position_location >= 0)
	{
		glEnableVertexAttribArray(position_location);
		glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
	}
	if (normal_location >= 0)
	{
		glEnableVertexAttribArray(normal_location);
		glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (const void *)(sizeof(float) * 3));
	}
	if (tex_coord_location >= 0)
	{
		glEnableVertexAttribArray(tex_coord_location);
		glVertexAttribPointer(tex_coord_location, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (const void *)(sizeof(float) * 6));
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.IndexBuffer);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	terrain.Mode = GL_TRIANGLE_STRIP;
	terrain.DrawArraysCount = 0;
	terrain.DrawElementsCount = indices.size();

	return terrain;
}

//-----------------------------------------
//----      Random trees planting      ----
//-----------------------------------------

void RandomTrees(const Terrain& terrain_geometry, glm::mat4* tree_model_matrixes, int tree_count, std::function<float(float, float, float)> callable) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> disArea(-50.0f, 50.0f - 1.0f);
	std::uniform_real_distribution<> disAngle(0.0f, 6.28f);
	std::uniform_real_distribution<> disGeneral(0.0f, 1.0f);
	for (size_t i = 0; i < tree_count; i++)
	{
		float x = disArea(gen);
		int xdata = static_cast<int>((x + 50) * 256 / 100);
		float z = disArea(gen);
		int zdata = static_cast<int>((z + 50) * 256 / 100);

		float y1 = terrain_geometry.height[xdata][zdata];
		float y2 = terrain_geometry.height[xdata + 1][zdata + 1];

		float dx = x - static_cast<int>(x);
		float dz = z - static_cast<int>(z);
		float t = sqrt(dx * dx + dz * dz) / sqrt(2.0f);
		float y = (1 - t) * y1 + t * y2;

		if (disGeneral(gen) > callable(x, y, z)) {
			i -= 1;
			continue;
		}

		glm::mat4 mat(1.0);
		mat = glm::translate(mat, glm::vec3(x, y * 32.0f, z));
		mat = glm::rotate(mat, -tan(y1 - terrain_geometry.height[xdata + 1][zdata]), glm::vec3(0.0, 0.0, 1.0));
		mat = glm::rotate(mat, -tan(y1 - terrain_geometry.height[xdata][zdata + 1]), glm::vec3(1.0, 0.0, 0.0));
		mat = glm::rotate(mat, static_cast<float>(disAngle(gen)), glm::vec3(0.0, 1.0, 0.0));
		tree_model_matrixes[i] = mat;
	}
}

//-----------------------------------------
//----      BLINKING CAMERA CLASS      ----
//-----------------------------------------

const float BlinkCamera::min_elevation = -1.5f;
const float BlinkCamera::max_elevation = 1.5f;
const float BlinkCamera::angle_sensitivity = 0.008f;
const float BlinkCamera::move_speed = 0.2f;

BlinkCamera::BlinkCamera(Terrain* terrain, float x, float z)
	: terrain(terrain), angle_direction(0.0f), angle_elevation(0.0f)
{
	eye_position.x = x;
	eye_position.z = z;
	eye_position.y = get_height(x, z);
	update_look_pos();
}

void BlinkCamera::OnMouseButtonChanged(int button, int state, int x, int y)
{
	
}

void BlinkCamera::update_look_pos() {
	look_position.x = eye_position.x + cosf(angle_elevation) * cosf(angle_direction);
	look_position.y = eye_position.y + -sinf(angle_elevation);
	look_position.z = eye_position.z + cosf(angle_elevation) * sinf(angle_direction);
}

float BlinkCamera::get_height(float x, float z) {
	int xi = static_cast<int>(round((eye_position.x / 50) * 128)) + 128;
	int zi = static_cast<int>(round((eye_position.z / 50) * 128)) + 128;
	xi = std::max(std::min(xi, 255), 0);
	zi = std::max(std::min(zi, 255), 0);
	return terrain->height[xi][zi] * 32.0f;
}

void BlinkCamera::OnMouseMoved(int dx, int dy)
{
	angle_direction += dx * angle_sensitivity;
	angle_elevation += dy * angle_sensitivity;

	// Clamp the results
	if (angle_elevation > max_elevation)    angle_elevation = max_elevation;
	if (angle_elevation < min_elevation)    angle_elevation = min_elevation;

	update_look_pos();
}

void BlinkCamera::Move()
{
	float vv = vel_w ? 1.0 : 0.0 + vel_s ? -1.0 : 0.0;
	float vu = vel_d ? 1.0 : 0.0 + vel_a ? -1.0 : 0.0;

	if (vv + vu == 0)
		return;

	float inAngle = atan2(vu, vv);

	eye_position.x += move_speed * cosf(angle_direction + inAngle);
	eye_position.z += move_speed * sinf(angle_direction + inAngle);
	eye_position.y = get_height(eye_position.x, eye_position.z);

	update_look_pos();
}

glm::vec3 BlinkCamera::GetEyePosition() const
{
	return eye_position;
}

glm::vec3 BlinkCamera::GetLookPosition() const
{
	return look_position;
}