#version 330

const int LIGHTS_COUNT = 2;

out vec4 final_color;

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

uniform sampler2D reflection_tex;

void main()
{
	// Reflection
	vec2 reflectOffset = inData.normal_ws.xz * 0.1;

	vec4 tex_color = texture(reflection_tex, (gl_FragCoord.xy) / textureSize(reflection_tex, 0) + reflectOffset);
	tex_color.a = 0.5;

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

		light += mat_ambient * lights[l].light_ambient_color * Ipow+
			mat_diffuse * lights[l].light_diffuse_color * Idiff * Ipow +
			mat_specular * lights[l].light_specular_color * Ispec * Ipow;
	}
	
	// Final
	final_color = vec4(max(tex_color.rgb, light.rgb), tex_color.a);
}
