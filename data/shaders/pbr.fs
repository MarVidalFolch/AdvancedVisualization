#define PI 3.14159265359
#define RECIPROCAL_PI 0.3183098861837697
#define epsilon 0.0000001

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
	float roughness;
	float metalness;
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
	pbr_mat.roughness = u_roughness_factor*texture2D(u_roughness_texture, v_uv).x;
	pbr_mat.metalness = u_metalness_factor*texture2D(u_metalness_texture, v_uv).x;
	
	pbr_mat.c_diff = mix(vec3(0.0), u_color.xyz, pbr_mat.metalness);
	pbr_mat.F0 = mix(u_color.xyz, vec3(0.04), pbr_mat.metalness);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float EpicNotesGeometricFunction(){
	float k = pow(pbr_mat.roughness + 1.0, 2.0)/8.0;
	vec3 N = vectors.N;
	vec3 V = vectors.V;
	vec3 L = vectors.L;
	float NdotL = max(dot(N,L), epsilon);
	float NdotV = max(dot(N,V), epsilon);
	
	// G_1(L)
	float g1_L = NdotL /(NdotL*(1.0-k)+k);
		
	// G_1(V)
	float g1_V = NdotV /(NdotV*(1.0-k)+k);
	
	return g1_L * g1_V;
}

float CookTorranceGeometricFunction(){
	vec3 N = vectors.N;
	vec3 H = vectors.H;
	vec3 V = vectors.V;
	
	float NdotH = max(dot(N, H), epsilon);
	float NdotV = max(dot(N, V), epsilon);
	float VdotH = max(dot(V, H), epsilon);
	float NdotL = max(dot(N, vectors.L), epsilon);
	
	float first_term = 2.0 * NdotH*NdotV / VdotH;
	float second_term = 2.0 * NdotH*NdotL / VdotH;
	
	float first_min = min(1.0, first_term);
	
	return min(first_min, second_term);
}

float BeckmanTowebrigdeDistributionFunction(){
	float alpha_sq = pow(pbr_mat.roughness,4.0);
	float NdotH_sq = pow(max(dot(vectors.N,vectors.H), epsilon),2.0);
	float denominator = pow(NdotH_sq*(alpha_sq-1.0)+1.0,2.0);
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
	float NdotL = clamp(dot(vectors.N,vectors.L),epsilon, 1.0);
	float NdotV = clamp(dot(vectors.N,vectors.V), epsilon, 1.0);
	
	vec3 specular_amount = F*G*D / (4.0*NdotL*NdotV);
	
	return f_lambert + specular_amount;
	
}

void main(){
	computeVectors();
	getMaterialProperties();
	gl_FragColor.xyz = u_light_color.xyz * getPixelColor();
	
}