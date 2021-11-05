const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

uniform samplerCube u_texture;

varying vec3 v_position;
varying vec3 v_world_position;

uniform vec3 u_camera_position;

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

void main()
{
    vec3 V = normalize(v_world_position - u_camera_position);
	
	vec3 hdre_texture = textureCube(u_texture, V).rgb;
	
	// Tonemapping
	vec3 hdre_texture_tm = toneMap(hdre_texture);
	
	// gamma
	vec3 pixel_color = linear_to_gamma(hdre_texture_tm);
	
    gl_FragColor.xyz = pixel_color;
}