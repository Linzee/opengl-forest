#version 330

const int TREE_COUNT = 1000;

in vec4 position;
in vec3 normal;
in vec2 tex_coord;

uniform mat4 model_matrix;
uniform float wind_height;
uniform float app_time;

uniform TreeData
{
       mat4 tree_model_matrix[TREE_COUNT];
};

uniform CameraData
{
	mat4 view_matrix;
	mat4 projection_matrix;
	vec3 eye_position;
};

out VertexData
{
	vec3 normal_ws;
	vec3 position_ws;
	vec2 tex_coord;
} outData;

void main()
{
	vec4 instance_pos = tree_model_matrix[gl_InstanceID] * model_matrix * position;
	
	float w = pow(position.y / wind_height, 3) * max(0.1, sin(gl_InstanceID / 17.0));
	float wx = w * sin(app_time * 0.7) * cos(app_time * 0.01);
	float wy = w * cos(app_time * 0.3) * sin(app_time * 0.43);
	instance_pos += vec4(wx, 0.0, wy, 0.0);

	outData.position_ws = vec3(instance_pos);
	
	// We transform the normal using model_matrix, not the transpose of its inverse (the normal matrix).
	// This is correct as long as the transformation contains only translations and rotations.
	outData.normal_ws = normalize(mat3(model_matrix) * normal);
	
	gl_ClipDistance[0] = outData.position_ws.y;

	outData.tex_coord = tex_coord;

	gl_Position = projection_matrix * view_matrix * instance_pos;
	
}
