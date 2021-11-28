uniform sampler2D u_texture;
uniform vec4 u_color;
uniform sampler3D u_volume_texture;
uniform float u_step_length;
uniform mat4 u_inv_model;
uniform float u_brightness;

uniform vec3 u_camera_position;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;

uniform sampler2D u_noise_texture;
uniform float u_noise_texture_width;

uniform sampler2D u_tf_texture;
uniform float u_classification_option;

uniform vec4 u_plane_parameters;
uniform bool u_apply_plane;

uniform float u_isovalue;
uniform bool u_apply_isosurface;

uniform float u_h;

const int MAX_ITERATIONS = 100000;


// PBR APPLICATION

#define PI 3.14159265359
#define RECIPROCAL_PI 0.3183098861837697
#define epsilon 0.0000001

uniform vec4 u_light_color;
uniform float u_light_intensity;
uniform vec3 u_light_pos;

uniform float u_roughness_factor;
uniform float u_metalness_factor;

uniform mat4 u_model;

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
	vec4 base_color;
	vec3 f_lambert;
	vec3 F0;	
}pbr_mat;

struct dotProducts
{
	float NdotL;
	float NdotV;
	float NdotH;
	float VdotH;
}dp;

void computeDotProducts(vec3 N, vec3 L, vec3 V, vec3 H){
	dp.NdotL = max(dot(N,L), epsilon);
	dp.NdotV = max(dot(N,V), epsilon);
	dp.NdotH = max(dot(N,H), epsilon);
	dp.VdotH = max(dot(V, H), epsilon);
}


void computeVectors(vec3 ray_position_local, vec3 normal){
	vec3 sample_pos_world = (u_model * vec4( ray_position_local, 0.0) ).xyz;
	
	// Light vector in local coordinates
	vectors.L = normalize(u_light_pos - sample_pos_world);
	
	// Eye vector or camera vector
	vectors.V = normalize(u_camera_position - sample_pos_world);
	
	// Normal vector
	vectors.N = (u_model * vec4( normal, 0.0) ).xyz;
	vectors.N = normalize(vectors.N);
	
	// Reflected ray 
	vectors.R = reflect(-vectors.V, vectors.N);
	vectors.R = normalize(vectors.R);
	
	// Half vector
	vectors.H = normalize(vectors.V + vectors.L);
	
	// Compute dot products of the vectors
	computeDotProducts(vectors.N, vectors.L, vectors.V, vectors.H);
}


void getMaterialProperties(){
	pbr_mat.roughness = u_roughness_factor;
	pbr_mat.metalness = u_metalness_factor;
	
	pbr_mat.base_color = vec4(1.0);//u_color;
	
	pbr_mat.f_lambert = mix(pbr_mat.base_color.rgb, vec3(0.0), pbr_mat.metalness) * RECIPROCAL_PI;
	pbr_mat.F0 = mix(vec3(0.04), pbr_mat.base_color.rgb, pbr_mat.metalness);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float EpicNotesGeometricFunction(){
	float k = pow(pbr_mat.roughness + 1.0, 2.0)/8.0;
	float NdotL = dp.NdotL;
	float NdotV = dp.NdotV;
	
	// G_1(L)
	float g1_L = NdotL /(NdotL*(1.0-k)+k);
		
	// G_1(V)
	float g1_V = NdotV /(NdotV*(1.0-k)+k);
	
	return g1_L * g1_V;
}

float BeckmanTowebrigdeDistributionFunction(){
	float alpha_sq = pow(pbr_mat.roughness,4.0);
	float NdotH_sq = pow(dp.NdotH,2.0);
	float denominator = pow(NdotH_sq*(alpha_sq-1.0)+1.0,2.0);
	return alpha_sq*RECIPROCAL_PI/denominator;

}


vec3 directLightCompute(){
	float NdotL = dp.NdotL;
	float NdotV = dp.NdotV;
	
	// PBR Specular
	vec3 F = FresnelSchlickRoughness(NdotL, pbr_mat.F0, pbr_mat.roughness);
	float G = EpicNotesGeometricFunction();
	float D = BeckmanTowebrigdeDistributionFunction();
	
	vec3 specular_amount = F*G*D / (4.0*NdotL*NdotV);
	
	vec3 pbr_term = pbr_mat.f_lambert + specular_amount;
	
	return pbr_term * u_light_color.xyz * u_light_intensity; 
	
}

// END PBR APPPICATION
uniform vec3 u_ka;
uniform vec3 u_kd;
uniform vec3 u_ks;
uniform float u_alpha_sh;

uniform vec3 u_ambient_light;
uniform vec3 u_light_diffuse;
uniform vec3 u_light_specular;

// PHONG
vec3 phongIllumination(){	
	// Normal vector
	vec3 N = vectors.N;
	
	// Negative light vector
	vec3 L = vectors.L;
		
	// N dot L
	float NdotL = dp.NdotL;
	
	// Reflected ray 
	vec3 R = reflect(-L, N);
	R = normalize(R);
	
	// Eye vector or camera vector
	vec3 V = vectors.V;
	
	// R dot V
	float RdotV = clamp(dot(R, V), 0.0, 1.0);
	
	// Ambient light term
	vec3 ka_ia = u_ka * u_ambient_light;
	
	// Diffuse ligth term
	vec3 kd_NdotL_id = u_kd * NdotL * u_light_diffuse;
	
	// Specular light term
	vec3 ks_RdotV_is = u_ks * pow(RdotV, u_alpha_sh) * u_light_specular;
	
	vec3 light = ka_ia + kd_NdotL_id + ks_RdotV_is;
	
	return light * u_light_color.xyz * u_light_intensity;
}


// END PHONG

vec3 worldCoordsToLocalCoords(vec4 world_coords){
	// Convert ray_position_local from world to local
	vec4 ray_position_local = u_inv_model*world_coords;
	
	// Such coordinates are in homogeneous coordinates, but we want cartesian coords
	// Then, we need to divide by the last coord, and remove the last coordinate
	return ray_position_local.xyz / ray_position_local.w;

}

vec3 worldCoordsToLocalCoordsVectors(vec4 world_vector){
	// It expects 3D world coords. The 4th component defines if it is a position or a vector
	// if(world_coords.w == 0) --> it's a vector
	return normalize(u_inv_model * world_vector).xyz;
}

vec3 localCoordsToTextureCoords(vec3 local_coords){
	// Convert ray_position_local from local to texture coords
	return (local_coords + 1.0) / 2.0;
}


vec3 addOffset(vec3 position, vec3 direction){
	vec2 uv = gl_FragCoord.xy / u_noise_texture_width;
	float offset = u_step_length*(texture2D(u_noise_texture, uv).x);
	return position + direction * offset;
}

bool isBeforePlane(vec3 position){
	vec3 plane_normal = normalize(u_plane_parameters.xyz);
	
	vec4 plane_params = vec4(plane_normal, u_plane_parameters.w);
	
	float result = dot(u_plane_parameters, vec4(position, 1.0));
	if(result > 0.0){
		return true;
	}
	return false;
}

vec3 computeGradient(vec3 ray_position_local){
	// X coord neightbors
	vec3 nx_r = vec3(ray_position_local.x + u_h, ray_position_local.y, ray_position_local.z);
	vec3 nx_l = vec3(ray_position_local.x - u_h, ray_position_local.y, ray_position_local.z);
	float d_nx_r = texture3D(u_volume_texture, localCoordsToTextureCoords(nx_r)).x;
	float d_nx_l = texture3D(u_volume_texture, localCoordsToTextureCoords(nx_l)).x;
	
	// Y coord neightbors
	vec3 ny_up = vec3(ray_position_local.x, ray_position_local.y + u_h, ray_position_local.z);
	vec3 ny_bt = vec3(ray_position_local.x, ray_position_local.y - u_h, ray_position_local.z);
	float d_ny_up = texture3D(u_volume_texture, localCoordsToTextureCoords(ny_up)).x;
	float d_ny_bt = texture3D(u_volume_texture, localCoordsToTextureCoords(ny_bt)).x;
	
	// Z coord neightbors
	vec3 nz_ah = vec3(ray_position_local.x, ray_position_local.y, ray_position_local.z + u_h);
	vec3 nz_bh = vec3(ray_position_local.x, ray_position_local.y, ray_position_local.z - u_h);
	float d_nz_ah = texture3D(u_volume_texture, localCoordsToTextureCoords(nz_ah)).x;
	float d_nz_bh = texture3D(u_volume_texture, localCoordsToTextureCoords(nz_bh)).x;
	
	// Compute each component of the gradient
	float g_x = d_nx_r - d_nx_l;
	float g_y = d_ny_up - d_ny_bt;
	float g_z = d_nz_ah - d_nz_bh;
	
	vec3 gradient = vec3(g_x,g_y,g_z)/(2.0*u_h);
	
	// Since it will be used as an estimator of the Normal, we normalize it.
	return normalize(gradient); 
}

void main()
{
	// 1. Ray setup
	// Pixel position in local coords
	vec3 ray_position_local = v_position;
	// Ray dircetion in world coords
	vec3 ray_direction = normalize(v_world_position - u_camera_position);
	// Ray direction in local coords
	ray_direction = worldCoordsToLocalCoordsVectors(vec4(ray_direction, 0.0));
	
	// step length in local coords
	vec3 ray_offset = u_step_length*ray_direction;
	
	// Offset to prevent jittering
	ray_position_local = addOffset(ray_position_local, ray_direction);
	
	vec3 sample_position = localCoordsToTextureCoords(ray_position_local);
	
	
	// Final color
	vec4 final_color = vec4(0.0);
	
	// Ray loop
	for(int i=0; i<MAX_ITERATIONS; i++){
		// 6. Early termination
		bvec3 bottom_condition = lessThan(sample_position, vec3(0.0));
		bvec3 top_condition = greaterThan(sample_position, vec3(1.0));
		
		if(any(top_condition) || any(bottom_condition))
		{
			break;
		}
		
		if (final_color.a >= 1.0){
			break;
		}
		
		// 2. Volume sampling
		float d = texture3D(u_volume_texture, sample_position).x;
		
		// Volume clipping and isosurface
		if((u_apply_plane && isBeforePlane(ray_position_local)) || (d < u_isovalue && u_classification_option >= 2.0)){
			ray_position_local += ray_offset;
			sample_position = localCoordsToTextureCoords(ray_position_local);
			continue;
		}

		vec4 sample_color = vec4(0.0);
		// 3. Classification
		if (u_classification_option == 0.0){  // part 1 of the lab
			sample_color = vec4(d,d,d,d);
		}
		else if (u_classification_option == 1.0){  // transfer function
			d = clamp(d, 0.01, 0.99);
			sample_color = texture2D(u_tf_texture, vec2(d, 0.5));
		}
		else if (u_classification_option == 2.0) {
			vec3 N = -computeGradient(ray_position_local);
			computeVectors(ray_position_local, N);
			getMaterialProperties();
			
			sample_color = vec4(directLightCompute(), 1.0);
		}
		else if (u_classification_option == 3.0){
			vec3 N = -computeGradient(ray_position_local);
			computeVectors(ray_position_local, N);
			
			sample_color = vec4(phongIllumination(), 1.0);
		}
		

		// 4. Composition
		final_color += localCoordsToTextureCoords(vec3(u_step_length)).x * (1.0 - final_color.a)*sample_color;
		

		// 5. Next sample
		ray_position_local += ray_offset;
		sample_position = localCoordsToTextureCoords(ray_position_local);
	}

	//7. Final color
	final_color.xyz *= u_brightness;
	gl_FragColor = final_color;

}