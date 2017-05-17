#version 330

const int LIGHTS_COUNT = 2;

out vec4 final_color;

const float triplanar_blend_sharpness = 1.0;

in VertexData
{
	vec3 normal_ws;
	vec3 position_ws;
	vec2 tex_coord;
} inData;

uniform CameraData
{
	mat4 view_matrix;
	mat4 projection_matrix;
	vec3 eye_position;
};

struct Light
{
	vec4 light_position;
	vec4 light_ambient_color;
	vec4 light_diffuse_color;
	vec4 light_specular_color;
	vec4 light_size;
};

uniform LightData
{
	Light lights[LIGHTS_COUNT];
};

uniform MaterialData
{
	uniform vec4 material_ambient_color;
	uniform vec4 material_diffuse_color;
	uniform vec4 material_specular_color;
	uniform float material_shininess;
};

uniform sampler2D grass_tex;
uniform sampler2D rocks_tex;

void main()
{
	
	// Difuse
    vec3 tex_color_x, tex_color_y, tex_color_z;
	vec2 texture_scale = vec2(1.0, 1.0);
	
	float m = 1 - dot(inData.normal_ws, vec3(0, 1, 0));

	if(inData.position_ws.y < 0.1) {
		m = 1;
	}

	if (m < 0.7) {
		tex_color_y = texture(grass_tex, inData.position_ws.xz * texture_scale).rgb;
		tex_color_x = texture(grass_tex, inData.position_ws.zy * texture_scale).rgb;
		tex_color_z = texture(grass_tex, inData.position_ws.xy * texture_scale).rgb;
	} else {
		tex_color_y = texture(rocks_tex, inData.position_ws.xz * texture_scale).rgb;
		tex_color_x = texture(rocks_tex, inData.position_ws.zy * texture_scale).rgb;
		tex_color_z = texture(rocks_tex, inData.position_ws.xy * texture_scale).rgb;
	}

	vec3 blendWeights = pow(abs(inData.normal_ws), vec3(triplanar_blend_sharpness, triplanar_blend_sharpness, triplanar_blend_sharpness));
	blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

	vec4 tex_color = vec4(tex_color_x * blendWeights.x + tex_color_y * blendWeights.y + tex_color_z * blendWeights.z, 1.0);
	
	// Lights
    vec3 N = normalize(inData.normal_ws);
	vec3 Eye = normalize(eye_position - inData.position_ws);

	vec4 mat_ambient = material_ambient_color * tex_color;
	vec4 mat_diffuse = material_diffuse_color * tex_color;
	vec4 mat_specular = material_specular_color;

	vec4 light = vec4(0.0, 0.0, 0.0, 0.0);

	for (int l=0; l < LIGHTS_COUNT; l++){
		vec3 L;
		if (lights[l].light_position.w == 0.0)
			L = normalize(lights[l].light_position.xyz);
		else
			L = normalize(lights[l].light_position.xyz - inData.position_ws);

		vec3 H = normalize(L + Eye);

		float Idiff = max(dot(N, L), 0.0);
		float Ispec = Idiff * pow(max(dot(N, H), 0.0), material_shininess);
		float Ipow = 1.0;
		if (lights[l].light_position.w != 0.0) {
			float d = distance(inData.position_ws, lights[l].light_position.xyz);
			Ipow = max(0, 1 - (d / lights[l].light_size.x));
		}

		light += mat_ambient * lights[l].light_ambient_color * Ipow +
			mat_diffuse * lights[l].light_diffuse_color * Idiff * Ipow +
			mat_specular * lights[l].light_specular_color * Ispec * Ipow;
	}

	// Final
	final_color = vec4(light.rgb, tex_color.a);
}
