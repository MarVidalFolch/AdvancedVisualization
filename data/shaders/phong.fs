uniform sampler2D u_texture;
uniform vec4 u_color;
uniform vec3 u_ka;
uniform vec3 u_kd;
uniform vec3 u_ks;
uniform float u_alpha_sh;

varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;

void main()
{
	
	gl_FragColor = clamp(u_color * texture(u_texture, v_uv), 0.0, 1.0);
}