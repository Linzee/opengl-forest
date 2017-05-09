#version 330

in vec4 position;
in vec3 normal;
in vec2 tex_coord;

uniform mat4 model_matrix;

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
	outData.position_ws = vec3(model_matrix * position);
	
	// No transformations applied!
	outData.normal_ws = normal;

	gl_ClipDistance[0] = outData.position_ws.y;

	outData.tex_coord = tex_coord;

	gl_Position = projection_matrix * view_matrix * model_matrix * position;
}
