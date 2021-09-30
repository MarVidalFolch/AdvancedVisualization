uniform sampler2D u_texture;
uniform vec4 u_color;
uniform vec3 u_ka;
uniform vec3 u_kd;
uniform vec3 u_ks;
uniform float u_alpha_sh;

uniform vec4 u_light_color;
uniform vec3 u_light_diffuse;
uniform vec3 u_light_specular;
uniform vec3 u_light_pos;
uniform float u_light_max_distance;
uniform vec3 u_ambient_light;

uniform vec3 u_camera_pos; // Hay que PASARLA 

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;


void main()
{
	vec3 N = normalize(v_normal);
	
	// Negative light vector
	vec3 L = normalize(u_light_pos - v_world_position);
		
	// N dot L
	float NdotL = clamp(dot(N, L), 0.0, 1.0);
	
	// Reflected ray 
	vec3 R = -reflect(L, N);
	R = normalize(R);
	
	// Eye vector or camera vector
	vec3 V = u_camera_pos - v_world_position;
	V = normalize(V);
	
	// R dot V
	float RdotV = clamp(dot(R, V), 0.0, 1.0);
	
	// Ambient light term
	vec3 ka_ia = u_ka * u_ambient_light;
	
	// Diffuse ligth term
	vec3 kd_NdotL_id = u_kd * NdotL * u_light_diffuse;
	
	// Specular light term
	vec3 ks_RdotV_is = u_ks * pow(RdotV, u_alpha_sh) * u_light_specular;
	
	vec4 light = vec4(ka_ia + kd_NdotL_id * ks_RdotV_is, 1.0);
	
	gl_FragColor = light * u_light_color * u_color;
	//gl_FragColor = vec4(1.0);
}