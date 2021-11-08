uniform sampler2D u_texture;
uniform vec4 u_color;
uniform sampler3D u_volume_texture;
uniform float u_ray_step;
uniform mat4 u_inv_model;

uniform vec3 u_camera_position;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;

vec3 worldCoordsToTextureCoords(vec3 world_coords){
	// Convert pixel_position from world to local
	vec4 pixel_position_local = u_inv_model*vec4(world_coords, 1.0);
	
	// Such coordinates are in homogeneous coordinates, but we want cartesian coords
	// Then, we need to divide by the last coord, and remove the last coordinate
	vec3 pixel_position_local_cart = pixel_position_local.xyz / pixel_position_local.z;
	
	// Convert pixel_position from local to texture coords
	return normalize((pixel_position_local_cart + 1.0) / 2.0);
}


void main()
{
	// 1. Ray setup
	vec3 pixel_position = v_world_position;
	vec3 ray_direction = normalize(pixel_position - u_camera_position);
	
	vec3 sample_position = worldCoordsToTextureCoords(pixel_position);
	
	// Final color
	vec4 final_color = vec4(0.0);
	
	// Get the initial density
	float d = texture3D(u_volume_texture, sample_position).x;
	
	// Ray loop
	for(int=0; i<MAX_ITERATIONS; i++){
		// 2. Volume sampling
		//...

		// 3. Classification
		//...

		// 4. Composition
		//...

		// 5. Next sample
		//...

		// 6. Early termination
		//...
	}

	//7. Final color
	//...
	//gl_FragColor.xyz = vec4(vec3(d), 1.0)*50;

}