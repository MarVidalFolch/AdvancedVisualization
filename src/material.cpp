#include "material.h"
#include "texture.h"
#include "application.h"
#include "extra/hdre.h"
#include "utils.h"
SkyboxMaterial* ReflectionMaterial::skybox = NULL;

StandardMaterial::StandardMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
}

StandardMaterial::~StandardMaterial()
{

}

void StandardMaterial::setUniforms(Camera* camera, Matrix44 model)
{
	//upload node uniforms
	//shader->enable();
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_time", Application::instance->time);
	shader->setUniform("u_output", Application::instance->output);

	shader->setUniform("u_color", color);
	shader->setUniform("u_exposure", Application::instance->scene_exposure);

	if (texture)
		shader->setUniform("u_texture", texture);		
}

void StandardMaterial::render(Mesh* mesh, Matrix44 model, Camera* camera)
{
	if (mesh && shader)
	{
		//enable shader
		shader->enable();

		//upload uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();
	}
}

void StandardMaterial::renderInMenu()
{
	ImGui::ColorEdit3("Color", (float*)&color); // Edit 3 floats representing a color
}

WireframeMaterial::WireframeMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
}

WireframeMaterial::~WireframeMaterial()
{

}

void WireframeMaterial::render(Mesh* mesh, Matrix44 model, Camera * camera)
{
	if (shader && mesh)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//enable shader
		shader->enable();

		//upload material specific uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

TextureMaterial::TextureMaterial(Texture* texture) {
	if (texture) {
		this->texture = texture;
	}
	else
	{
		this->texture = Texture::getWhiteTexture();
	}

	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
}


TextureMaterial::TextureMaterial(char* folder_texture, Texture* texture) {
	if (texture) {
		this->texture = texture;
	}
	else
	{
		this->texture = Texture::getWhiteTexture();
	}

	this->folder_index_texture = getIndex(folder_names_texture, folder_texture);

	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
}

void TextureMaterial::renderInMenu() {
	bool changed = false;
	changed |= ImGui::Combo("Sphere texture", (int*)&this->folder_index_texture, "BLUE_NOISE\0brdfLUT\0ALEX_MAR\0");
	if (changed){ 
		textureUpdate();
	}
}

void TextureMaterial::textureUpdate() {
	if (folder_index_texture >= std::size(folder_names_texture)) {
		texture = Texture::getWhiteTexture();
	}
	texture = Texture::Get(folder_names_texture[folder_index_texture]);
}

PhongMaterial::PhongMaterial(char* folder_names_texture, Vector4 color, Vector3 ka, Vector3 kd, Vector3 ks, float alpha_sh, Shader* shader, Texture* texture) : TextureMaterial(texture) {
	if (shader == NULL) {
		this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/phong.fs");
	}
	else {
		this->shader = shader;
	}

	this->color = color;

	this->ka = ka;
	this->kd = kd;
	this->ks = ks;
	this->alpha_sh = alpha_sh;
}

void PhongMaterial::setUniforms(Camera* camera, Matrix44 model) {
	StandardMaterial::setUniforms(camera, model);
	shader->setUniform("u_ka", ka);
	shader->setUniform("u_kd", kd);
	shader->setUniform("u_ks", ks);
	shader->setUniform("u_alpha_sh", alpha_sh);
}

void PhongMaterial::renderInMenu() {
	ImGui::DragFloat3("Ambient reflection (ka)",(float*)&this->ka, 0.005f, 0.0f, 1.0f);
	ImGui::DragFloat3("Diffuse reflection (kd)", (float*)&this->kd, 0.005f, 0.0f, 1.0f);
	ImGui::DragFloat3("Specular reflection (ks)", (float*)&this->ks, 0.005f, 0.0f, 1.0f);
	ImGui::DragFloat("Alpha shinning", (float*)&this->alpha_sh, 0.01f, 0.01f, 50.0f);

	bool changed = false;
	changed |= ImGui::Combo("Sphere texture", (int*)&this->folder_index_texture, "BLUE_NOISE\0brdfLUT\0ALEX_MAR\0");
	if (changed) {
		textureUpdate();
	}
}

SkyboxMaterial::SkyboxMaterial(char* folder_texture, Texture* texture, Shader* shader) : TextureMaterial(texture) {
	if (shader == NULL) {
		this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/skybox.fs");
	}
	else {
		this->shader = shader;
	}
	this->folder_index = getIndex(folder_names, folder_texture);
}

void SkyboxMaterial::renderInMenu() {
	bool changed = false;
	changed |= ImGui::Combo("Skybox texture", (int*)&this->folder_index, "SNOW\0CITY\0DRAGON");
	if (changed) {
		textureSkyboxUpdate();
	}

}

void SkyboxMaterial::textureSkyboxUpdate() {
	texture->cubemapFromImages(folder_names[folder_index]);
}


ReflectionMaterial::ReflectionMaterial(SkyboxMaterial* skybox){
	this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/reflection.fs");
	if (ReflectionMaterial::skybox == NULL) {
		ReflectionMaterial::skybox = skybox;
	}
}

