uniform samplerCube u_texture;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
uniform vec3 u_camera_position;

void main()
{
    vec3 N = normalize(v_normal);
	vec3 V = normalize(v_world_position - u_camera_position); 
	vec3 R = reflect(V,N);
    gl_FragColor = textureCube(u_texture, R);
}

