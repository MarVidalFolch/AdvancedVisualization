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

const int MAX_ITERATIONS = 32;

vec3 worldCoordsToTextureCoords(vec3 world_coords){
	// Convert pixel_position from world to local
	vec4 pixel_position_local = u_inv_model*vec4(world_coords, 1.0);
	
	// Such coordinates are in homogeneous coordinates, but we want cartesian coords
	// Then, we need to divide by the last coord, and remove the last coordinate
	vec3 pixel_position_local_cart = pixel_position_local.xyz / pixel_position_local.w;
	
	// Convert pixel_position from local to texture coords
	return normalize((pixel_position_local_cart + 1.0) / 2.0);
}


void main()
{
	// 1. Ray setup
	vec3 pixel_position = v_world_position;
	vec3 ray_direction = normalize(pixel_position - u_camera_position);
	vec3 ray_offset = u_step_length*ray_direction;
	
	vec3 sample_position = worldCoordsToTextureCoords(pixel_position);
	
	// Final color
	vec4 final_color = vec4(0.0);
	
	// Ray loop
	for(int i=0; i<MAX_ITERATIONS; i++){
		// 2. Volume sampling
		float d = texture3D(u_volume_texture, sample_position).x;

		// 3. Classification
		vec4 sample_color = vec4(d,d,d,d);

		// 4. Composition
		final_color += u_step_length * (1.0 - final_color.a)*sample_color;

		// 5. Next sample
		pixel_position += ray_offset;
		sample_position = worldCoordsToTextureCoords(pixel_position);
		
		// 6. Early termination
		bvec3 bottom_condition = greaterThan(sample_position, vec3(0.0));
		bvec3 top_condition = lessThan(sample_position, vec3(1.0));
		
		//if(all(top_condition) && all(bottom_condition))
		//{
		//	break;
		//}
		
		if ( final_color.a >= 1.0){
			break;
		}
	}

	//7. Final color
	gl_FragColor = final_color*u_brightness;

}