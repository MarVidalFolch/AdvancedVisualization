uniform samplerCube u_texture;

varying vec3 v_position;
varying vec3 v_world_position;

uniform vec3 u_camera_pos;

void main()
{
    vec3 V = -normalize(u_camera_pos - v_world_position);
    gl_FragColor = texture(u_texture, V);
}