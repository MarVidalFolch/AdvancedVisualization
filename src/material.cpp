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

SkyboxMaterial::SkyboxMaterial(char* folder_texture, std::vector<Texture*> hdre_versions, Texture* texture, Shader* shader) : TextureMaterial(texture) {
	if (shader == NULL) {
		this->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/skybox.fs");
	}
	else {
		this->shader = shader;
	}
	this->folder_index = getIndex(folder_names, folder_texture);
	this->hdre_versions = hdre_versions;
}

void SkyboxMaterial::renderInMenu() {
	bool changed = false;
	changed |= ImGui::Combo("Skybox texture", (int*)&this->folder_index, "PISA\0PANORAMA\0STUDIO\0");
	if (changed) {
		textureSkyboxUpdate();
	}

}

void SkyboxMaterial::textureSkyboxUpdate() {
	HDRE* hdre = HDRE::Get(folder_names[folder_index]);

	// There are 5 levels: the original + 5 blurred versions
	for (unsigned int LEVEL = 0; LEVEL < 6; LEVEL = LEVEL + 1) {
		hdre_versions[LEVEL]->cubemapFromHDRE(hdre, LEVEL);
	}

	texture = hdre_versions[0];
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
	this->is_ao_texture = false;
	this->is_op_texture = false;
	this->ibl_scale = 1.0f;
	this->direct_scale = 1.0f;

}

void PBRMaterial::setUniforms(Camera* camera, Matrix44 model) {
	StandardMaterial::setUniforms(camera, model);
	shader->setUniform("u_roughness_texture", roughness_texture, (int)TextureSlots::ROUGHNESS);
	shader->setUniform("u_metalness_texture", metalness_texture, (int)TextureSlots::METALNESS);
	shader->setUniform("u_normal_texture", normal_texture, (int)TextureSlots::NORMAL);
	shader->setUniform("u_albedo_texture", albedo_texture, (int)TextureSlots::ALBEDO);
	if(is_ao_texture)
		shader->setUniform("u_ao_texture", ambient_occlusion_texture, (int)TextureSlots::AO);
	if (is_op_texture) {
		shader->setUniform("u_oppacity_texture", oppacity_texture, (int)TextureSlots::OPPACITY);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	shader->setUniform("u_roughness_factor", roughness_factor);
	shader->setUniform("u_metalness_factor", metalness_factor);
	shader->setUniform("u_is_ao", is_ao_texture);
	shader->setUniform("u_is_oppacity", is_op_texture);

	// HDRE environment
	shader->setUniform("u_hdre_texture_original", hdre_versions_environment[0], (int)TextureSlots::HDRE_ORIG);
	shader->setUniform("u_hdre_texture_prem_0", hdre_versions_environment[1], (int)TextureSlots::HDRE_L0);
	shader->setUniform("u_hdre_texture_prem_1", hdre_versions_environment[2], (int)TextureSlots::HDRE_L1);
	shader->setUniform("u_hdre_texture_prem_2", hdre_versions_environment[3], (int)TextureSlots::HDRE_L2);
	shader->setUniform("u_hdre_texture_prem_3", hdre_versions_environment[4], (int)TextureSlots::HDRE_L3);
	shader->setUniform("u_hdre_texture_prem_4", hdre_versions_environment[5], (int)TextureSlots::HDRE_L4);

	// BRDF LUT
	shader->setUniform("u_brdf_lut", brdfLUT_texture, (int)TextureSlots::BRDF_LUT);

	// Control parameters
	shader->setUniform("u_ibl_scale", ibl_scale);
	shader->setUniform("u_direct_scale", direct_scale);

	// Output control
	shader->setUniform("u_output", Application::instance->output);

}

void PBRMaterial::renderInMenu() {
	ImGui::ColorEdit4("Color", (float*)&this->color);
	ImGui::SliderFloat("Roughness factor", &this->roughness_factor, 0.0f, 2.0f);
	ImGui::SliderFloat("Metalness factor", &this->metalness_factor, 0.0f, 1.0f);
	ImGui::SliderFloat("IBL scale", &this->ibl_scale, 0.0f, 1.0f);
	ImGui::SliderFloat("Direct light scale", &this->direct_scale, 0.0f, 1.0f);


}

void PBRMaterial::render(Mesh* mesh, Matrix44 model, Camera* camera) {
	if (is_op_texture) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	StandardMaterial::render(mesh, model, camera);
	
	glDisable(GL_CULL_FACE);
}

VolumeMaterial::VolumeMaterial(Texture* volume_texture, float step_length) {
	this->volume_texture = volume_texture;
	this->step_length = step_length;
	this->brightness = 5.0;
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/volume.fs");
}

void VolumeMaterial::setUniforms(Camera* camera, Matrix44 model) {
	StandardMaterial::setUniforms(camera, model);

	shader->setUniform("u_volume_texture", volume_texture, (int)TextureSlots::VOLUME);
	shader->setUniform("u_step_length", this->step_length);
	shader->setUniform("u_brightness", this->brightness);
	Matrix44 inv_model = model;
	inv_model.inverse();
	shader->setUniform("u_inv_model", inv_model);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

}
/*
void VolumeMaterial::computeStepLength(Matrix44 model) 

	float ratio = 2.0 / 0.014; // ratio found
	float s_width = (model * Vector4(1.0, 0.0, 0.0, 0.0)).x;
	float s_height = (model * Vector4(0.0, 1.0, 0.0, 0.0)).y;
	float s_depth = (model * Vector4(0.0, 0.0, 1.0, 0.0)).z;
	step_length = Vector3(2.0 * s_width / ratio, 2.0 * s_height / ratio, 2.0 * s_depth / ratio);
}
*/


void VolumeMaterial::renderInMenu() {
	StandardMaterial::renderInMenu();
	ImGui::SliderFloat("Step length", &this->step_length, 0.0f, 0.30f);
	ImGui::SliderFloat("Brightness", &this->brightness, 0.0f, 50.0f);

}