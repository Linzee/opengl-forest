#version 330

in vec4 position;
in vec3 normal;
in vec2 tex_coord;

uniform mat4 model_matrix;
uniform float app_time;

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

uniform sampler2D water_normal_tex;

void main()
{	
	vec3 moved_normal = texture(water_normal_tex, tex_coord.st * 10 + vec2(app_time, app_time)).rbg - vec3(0.5, 0.5, 0.5);
	vec4 moved_pos = position + vec4(moved_normal * 0.3, 0.0) * 0.05;
	
	outData.position_ws = vec3(model_matrix * moved_pos);
	outData.normal_ws = moved_normal;
	
	gl_ClipDistance[0] = outData.position_ws.y;
	
	outData.tex_coord = tex_coord;

	gl_Position = projection_matrix * view_matrix * model_matrix * moved_pos;
}
