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

	if (texture!=NULL)
		shader->setUniform("u_texture", texture, (int)TextureSlots::ALBEDO);
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


TextureMaterial::TextureMaterial(char* filename_texture, Texture* texture) {
	if (texture) {
		this->texture = texture;
	}
	else
	{
		this->texture = Texture::getWhiteTexture();
	}

	this->filename_index_texture = getIndex(filenames_texture, filename_texture);

	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
}

void TextureMaterial::renderInMenu() {
	bool changed = false;
	changed |= ImGui::Combo("Sphere texture", (int*)&this->filename_index_texture, "BLUE_NOISE\0brdfLUT\0ALEX_MAR\0WHITE\0");
	if (changed){ 
		textureUpdate();
	}
}

void TextureMaterial::textureUpdate() {
	if (filename_index_texture >= std::size(filenames_texture)) {
		texture = Texture::getWhiteTexture();
		return;
	}
	texture = Texture::Get(filenames_texture[filename_index_texture]);
}

PhongMaterial::PhongMaterial(char* filename_texture, Vector4 color, Vector3 ka, Vector3 kd, Vector3 ks, float alpha_sh, Shader* shader, Texture* texture) : TextureMaterial(filename_texture, texture) {
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

	TextureMaterial::renderInMenu();
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

void ReflectionMaterial::setUniforms(Camera* camera, Matrix44 model)
{
	StandardMaterial::setUniforms(camera, model);

	if (ReflectionMaterial::skybox != NULL)
		shader->setUniform("u_texture", skybox->texture);
}

PBRMaterial::PBRMaterial(char* filename_texture, Texture* texture) : TextureMaterial(texture) {
	this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/pbr.fs");
	
	//this->folder_index = getIndex(folder_names, folder_texture);
}

PBRMaterial::PBRMaterial(float roughness_factor, float metalness_factor) {
	this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/pbr.fs");
	
	this->roughness_factor = roughness_factor;
	this->metalness_factor= metalness_factor;

}

void PBRMaterial::setUniforms(Camera* camera, Matrix44 model) {
	StandardMaterial::setUniforms(camera, model);
	shader->setUniform("u_roughness_texture", roughness_texture, (int)TextureSlots::ROUGHNESS);
	shader->setUniform("u_metalness_texture", metalness_texture, (int)TextureSlots::METALNESS);
	shader->setUniform("u_albedo_texture", albedo_texture, (int)TextureSlots::ALBEDO);
	shader->setUniform("u_roughness_factor", roughness_factor);
	shader->setUniform("u_metalness_factor", metalness_factor);

	// HDRE environment
	shader->setUniform("u_hdre_texture_original", hdre_versions_environment[0], (int)TextureSlots::HDRE_ORIG);
	shader->setUniform("u_hdre_texture_prem_0", hdre_versions_environment[1], (int)TextureSlots::HDRE_L0);
	shader->setUniform("u_hdre_texture_prem_1", hdre_versions_environment[2], (int)TextureSlots::HDRE_L1);
	shader->setUniform("u_hdre_texture_prem_2", hdre_versions_environment[3], (int)TextureSlots::HDRE_L2);
	shader->setUniform("u_hdre_texture_prem_3", hdre_versions_environment[4], (int)TextureSlots::HDRE_L3);
	shader->setUniform("u_hdre_texture_prem_4", hdre_versions_environment[5], (int)TextureSlots::HDRE_L4);

	/*for (int i = 0; i < hdre_versions_environment.size() - 1; i++) {
		std::string texture_name = "u_hdre_texture_prem_" + std::to_string(i);
		const char* final_name = texture_name.c_str();
		shader->setUniform(final_name, hdre_versions_environment[i+1], (int)TextureSlots::HDRE_L0 + i);
		std::cout << texture_name + "\n";
		std::cout << (int)TextureSlots::HDRE_L0 + i;
		std::cout << "\n\n";
	}
	*/
	// BRDF LUT
	shader->setUniform("u_brdf_lut", brdfLUT_texture, (int)TextureSlots::BRDF_LUT);
}

void PBRMaterial::renderInMenu() {
	ImGui::DragFloat("Roughness factor", &this->roughness_factor, 0.0025f, 0.0f, 2.0f);
	ImGui::DragFloat("Metalness factor", &this->metalness_factor, 0.0025f, 0.0f, 1.0f);
	//ImGui::DragFloat3("Color", &this->color)


}