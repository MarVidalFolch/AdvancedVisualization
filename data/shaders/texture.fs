uniform sampler2D u_texture;
uniform vec4 u_color;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;

void main()
{
	gl_FragColor = clamp(u_color * texture2D(u_texture, v_uv), 0.0, 1.0);
}