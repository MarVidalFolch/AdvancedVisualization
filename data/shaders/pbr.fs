#define PI 3.14159265359
#define RECIPROCAL_PI 0.3183098861837697

uniform sampler2D u_roughness_texture;
uniform float u_roughness_factor;
uniform sampler2D u_metalness_texture;
uniform float u_metalness_factor;
uniform vec4 u_color;

uniform vec4 u_light_color;
uniform vec3 u_light_intensity;
uniform vec3 u_light_pos;
uniform vec3 u_ambient_light;

uniform vec3 u_camera_position;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;

struct Vectors{
	vec3 V;
	vec3 N;
	vec3 R;
	vec3 H;
	vec3 L;
}vectors;

struct PBRMat
{
	vec4 roughness;
	vec4 metalness;
	vec3 c_diff;
	vec3 F0;	
}pbr_mat;

void computeVectors(){
	// Light vector
	vectors.L = normalize(u_light_pos - v_world_position);
	
	// Eye vector or camera vector
	vectors.V = normalize(u_camera_position - v_world_position);
	
	// Normal vector
	vectors.N = normalize(v_normal);
	
	// Reflected ray 
	vectors.R = reflect(-vectors.L, vectors.N);
	vectors.R = normalize(vectors.R);
	
	// Half vector
	vectors.H = normalize(vectors.V + vectors.L);
}

void getMaterialProperties(){
	pbr_mat.roughness = u_roughness_factor*texture2D(u_roughness_texture, v_uv);
	pbr_mat.metalness = u_metalness_factor*texture2D(u_metalness_texture, v_uv);
	
	pbr_mat.c_diff = mix(vec3(0.0), u_color.xyz, u_metalness_factor);
	pbr_mat.F0 = mix(u_color.xyz, vec3(0.04), u_metalness_factor);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 EpicNotesGeometricFunction(){
	float k = pow(pbr_mat.roughness + 1, vec4(2))/8;
	vec3 N = vectors.N;
	vec3 V = vectors.V;
	vec3 L = vectors.L;
	float NdotL = max(dot(N,L), 0.0);
	float NdotV = max(dot(N,V), 0.0);
	
	// G_1(L)
	vec3 g1_L = NdotL /(NdotL*(1-k)+k);
		
	// G_1(V)
	vec3 g1_V = NdotV /(NdotV*(1-k)+k);
	
	return g1_L * g1_V;
}

vec3 BeckmanTowebrigdeDistributionFunction(){
	float alpha_sq = pow(pbr_mat.roughness,vec4(4));
	float NdotH_sq = pow(max(dot(vectors.N,vectors.H), 0.0),2);
	float denominator = pow(NdotH_sq*(alpha_sq-1)+1,2);
	return alpha_sq*RECIPROCAL_PI/denominator;

}

vec3 getPixelColor(){
	// PBR diffuse
	vec3 f_lambert = pbr_mat.c_diff * RECIPROCAL_PI;
	
	// PBR Specular
	float cosTheta = max(dot(vectors.N, vectors.L), 0.0);
	vec3 F = FresnelSchlickRoughness(cosTheta, pbr_mat.F0, pbr_mat.roughness);
	float G = EpicNotesGeometricFunction();
	float D = BeckmanTowebrigdeDistributionFunction();
	float NdotL = max(dot(vectors.N,vectors.L), 0.0);
	float NdotV = max(dot(vectors.N,vectors.V), 0.0);
	
	return F*G*D/(4*NdotL*NdotV);
	
}

void main(){
	computeVectors();
	getMaterialProperties();
	gl_FragColor.xyz = getPixelColor();
	
}