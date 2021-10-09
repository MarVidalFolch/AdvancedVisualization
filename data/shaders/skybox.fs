uniform samplerCube u_texture;

varying vec3 v_position;
varying vec3 v_world_position;

uniform vec3 u_camera_position;

void main()
{
    vec3 V = normalize(v_world_position - u_camera_position);
    gl_FragColor = textureCube(u_texture, V);
}