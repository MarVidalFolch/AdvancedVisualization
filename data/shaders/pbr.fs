#define PI 3.14159265359
#define RECIPROCAL_PI 0.3183098861837697
#define epsilon 0.0000001
const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// LUT
uniform sampler2D u_brdf_lut;

// HDRE environmnent
uniform samplerCube u_hdre_texture_original;
uniform samplerCube u_hdre_texture_prem_0;
uniform samplerCube u_hdre_texture_prem_1;
uniform samplerCube u_hdre_texture_prem_2;
uniform samplerCube u_hdre_texture_prem_3;
uniform samplerCube u_hdre_texture_prem_4;

uniform sampler2D u_roughness_texture;
uniform float u_roughness_factor;
uniform sampler2D u_metalness_texture;
uniform float u_metalness_factor;
uniform sampler2D u_albedo_texture;
uniform sampler2D u_normal_texture;
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
	vec4 base_color;
	vec3 c_diff;
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

//Javi Agenjo Snipet for Bump Mapping
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

vec3 perturbNormal( vec3 N, vec3 V, vec2 texcoord, vec3 normal_pixel ){
	#ifdef USE_POINTS
	return N;
	#endif

	// assume N, the interpolated vertex normal and
	// V, the view vector (vertex to eye)
	
	normal_pixel = normal_pixel * 255./127. - 128./127.;
	mat3 TBN = cotangent_frame(N, V, texcoord);
	return normalize(TBN * normal_pixel);
}

void computeVectors(){
	// Light vector
	vectors.L = normalize(u_light_pos - v_world_position);
	
	// Eye vector or camera vector
	vectors.V = normalize(u_camera_position - v_world_position);
	
	// Normal vector
	vec3 normal_pixel = texture2D(u_normal_texture, v_uv).xyz;
	vectors.N = perturbNormal( v_normal, vectors.V, v_uv, normal_pixel );
	
	// Reflected ray 
	vectors.R = reflect(-vectors.V, vectors.N);
	vectors.R = normalize(vectors.R);
	
	// Half vector
	vectors.H = normalize(vectors.V + vectors.L);
	
	// Compute dot products of the vectors
	computeDotProducts(vectors.N, vectors.L, vectors.V, vectors.H);
}

// degamma
vec3 gamma_to_linear(vec3 color)
{
	return pow(color, vec3(GAMMA));
}

// gamma
vec3 linear_to_gamma(vec3 color)
{
	return pow(color, vec3(INV_GAMMA));
}

void getMaterialProperties(){
	pbr_mat.roughness = u_roughness_factor*texture2D(u_roughness_texture, v_uv).x;
	pbr_mat.metalness = u_metalness_factor*texture2D(u_metalness_texture, v_uv).x;
	
	pbr_mat.base_color = texture2D(u_albedo_texture, v_uv);
	
	pbr_mat.base_color.xyz = gamma_to_linear(pbr_mat.base_color.xyz);
	
	pbr_mat.c_diff = mix(pbr_mat.base_color.rgb, vec3(0.0), pbr_mat.metalness);
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

float CookTorranceGeometricFunction(){	
	float NdotH = dp.NdotH;
	float NdotV = dp.NdotV;
	float VdotH = dp.VdotH;
	float NdotL = dp.NdotL;
	
	float first_term = 2.0 * NdotH*NdotV / VdotH;
	float second_term = 2.0 * NdotH*NdotL / VdotH;
	
	float first_min = min(1.0, first_term);
	
	return min(first_min, second_term);
}

float BeckmanTowebrigdeDistributionFunction(){
	float alpha_sq = pow(pbr_mat.roughness,4.0);
	float NdotH_sq = pow(dp.NdotH,2.0);
	float denominator = pow(NdotH_sq*(alpha_sq-1.0)+1.0,2.0);
	return alpha_sq*RECIPROCAL_PI/denominator;

}

vec3 getPixelColor(){
	float NdotL = dp.NdotL;
	float NdotV = dp.NdotV;

	// PBR diffuse
	vec3 f_lambert = pbr_mat.c_diff * RECIPROCAL_PI;
	
	// PBR Specular
	vec3 F = FresnelSchlickRoughness(NdotL, pbr_mat.F0, pbr_mat.roughness);
	float G = EpicNotesGeometricFunction();
	float D = BeckmanTowebrigdeDistributionFunction();
	
	vec3 specular_amount = F*G*D / (4.0*NdotL*NdotV);
	
	return f_lambert + specular_amount;
	
}

vec3 getReflectionColor(vec3 r, float roughness)
{
	float lod = roughness * 5.0;

	vec4 color;

	if(lod < 1.0) color = mix( textureCube(u_hdre_texture_original, r), textureCube(u_hdre_texture_prem_0, r), lod );
	else if(lod < 2.0) color = mix( textureCube(u_hdre_texture_prem_0, r), textureCube(u_hdre_texture_prem_1, r), lod - 1.0 );
	else if(lod < 3.0) color = mix( textureCube(u_hdre_texture_prem_1, r), textureCube(u_hdre_texture_prem_2, r), lod - 2.0 );
	else if(lod < 4.0) color = mix( textureCube(u_hdre_texture_prem_2, r), textureCube(u_hdre_texture_prem_3, r), lod - 3.0 );
	else if(lod < 5.0) color = mix( textureCube(u_hdre_texture_prem_3, r), textureCube(u_hdre_texture_prem_4, r), lod - 4.0 );
	else color = textureCube(u_hdre_texture_prem_4, r);

	return color.rgb;
}


// Uncharted 2 tone map
// see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 toneMapUncharted2Impl(vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 toneMapUncharted(vec3 color)
{
    const float W = 11.2;
    color = toneMapUncharted2Impl(color * 2.0);
    vec3 whiteScale = 1.0 / toneMapUncharted2Impl(vec3(W));
    return color * whiteScale;
}

vec3 toneMap(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main(){
	computeVectors();
	getMaterialProperties();
	
	// PBR direct light
	vec3 pbr_term = getPixelColor();
	
	// IBL indirect light
	vec3 specularSample = getReflectionColor(vectors.R, pbr_mat.roughness);
	
	float NdotV = clamp(dot(vectors.N,vectors.V), 0.0, 1.0);
	
	vec3 brdf2D = texture2D(u_brdf_lut, vec2(NdotV, pbr_mat.roughness)).xyz;
	
	float cosTheta = max(dot(vectors.N, vectors.L), 0.0);
	vec3 F = FresnelSchlickRoughness(cosTheta, pbr_mat.F0, pbr_mat.roughness);
	vec3 SpecularBRDF = F * brdf2D.x + brdf2D.y;
	vec3 SpecularIBL = specularSample * SpecularBRDF;
	
	vec3 diffuseSample = getReflectionColor(vectors.N, pbr_mat.roughness);
	vec3 difusseColor = pbr_mat.c_diff;
	vec3 DiffuseIBL = diffuseSample * difusseColor;
	
	DiffuseIBL *= (1.0-F);
	
	vec3 ibl_term = SpecularIBL + DiffuseIBL;
	
	
	// Final light
	vec3 light = u_light_intensity * u_light_color.xyz * pbr_term * dp.NdotL + ibl_term;
	
	vec3 pixelColor = toneMap(light);
	
	pixelColor = linear_to_gamma(pixelColor);
	
	gl_FragColor.xyz = pixelColor;
	
}