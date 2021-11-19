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

const int MAX_ITERATIONS = 256;

vec3 worldCoordsToLocalCoords(vec4 world_coords){
	// Convert pixel_position from world to local
	vec4 pixel_position_local = u_inv_model*world_coords;
	
	// Such coordinates are in homogeneous coordinates, but we want cartesian coords
	// Then, we need to divide by the last coord, and remove the last coordinate
	return pixel_position_local.xyz / pixel_position_local.w;

}

vec3 worldCoordsToLocalCoordsVectors(vec4 world_vector){
	// It expects 3D world coords. The 4th component defines if it is a position or a vector
	// if(world_coords.w == 0) --> it's a vector
	return normalize(u_inv_model * world_vector).xyz;
}

vec3 localCoordsToTextureCoords(vec3 local_coords){
	// Convert pixel_position from local to texture coords
	return (local_coords + 1.0) / 2.0;
}


vec3 addOffset(vec3 coords){
	vec2 uv = gl_FragCoord.xy / u_noise_texture_width;
	vec3 offset = texture2D(u_noise_texture, uv).xyz;
	return normalize(coords + offset);
}

void main()
{
	// 1. Ray setup
	// Pixel position in local coords
	vec3 pixel_position = v_position;
	// Ray dircetion in world coords
	vec3 ray_direction = normalize(v_world_position - u_camera_position);
	// Ray direction in local coords
	ray_direction = worldCoordsToLocalCoordsVectors(vec4(ray_direction, 0.0));
	ray_direction = addOffset(ray_direction);
	
	// step length in local coords
	vec3 ray_offset = u_step_length*ray_direction;
	
	vec3 sample_position = localCoordsToTextureCoords(pixel_position);
	
	
	// Final color
	vec4 final_color = vec4(0.0);
	
	// Ray loop
	for(int i=0; i<MAX_ITERATIONS; i++){
		// 2. Volume sampling
		float d = texture3D(u_volume_texture, sample_position).x;

		// 3. Classification
		vec4 sample_color = vec4(d,d,d,d);

		// 4. Composition
		final_color += localCoordsToTextureCoords(vec3(u_step_length)).x * (1.0 - final_color.a)*sample_color;

		// 5. Next sample
		pixel_position += ray_offset;
		sample_position = localCoordsToTextureCoords(pixel_position);
		
		// 6. Early termination
		bvec3 bottom_condition = lessThan(sample_position, vec3(0.0));
		bvec3 top_condition = greaterThanEqual(sample_position, vec3(1.0));
		
		if(any(top_condition) || any(bottom_condition))
		{
			break;
		}
		
		if ( final_color.a >= 1.0){
			break;
		}
	}

	//7. Final color
	gl_FragColor = final_color*u_brightness;

}