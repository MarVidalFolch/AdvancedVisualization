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

VolumeMaterial::VolumeMaterial(std::vector<Texture*> volume_textures, Texture* noise_texture, std::vector<Texture*> tf_textures) {
	// Volume
	this->textures_volume_index = VolumeOption::FOOT;
	this->volume_texture = volume_textures[(int)this->textures_volume_index];
	this->volume_textures = volume_textures;
	// Ray params
	this->step_length = 0.048;
	this->brightness = 0.6;
	// Jittering
	this->noise_texture = noise_texture;
	// transfer function
	this->tf_texture = tf_textures[(int)this->textures_volume_index];
	this->tf_textures = tf_textures;

	this->classification_option = ClassificationOption::PART1;
	
	// Volume clipping plane
	this->plane_parameters = Vector4(-1.0, 0.0, 0.0, 0.0);
	this->apply_plane = false;
	// Isosurface 
	this->isovalue = 0.21;
	// Gradient step estimator
	this->h = 0.19;

	// Light PBR params
	this->light_position = Vector3(10.0, 10.0, 0.0);
	this->light_intentsity = 2.0;
	this->light_color = Vector4(0.8, 0.0, 0.5, 1.0);

	// Material PBR params
	this->roughness = 0.5;
	this->metalness = 1.0;

	// Light Phong Params
	this->ka = Vector3(0.3, 0.3, 0.3);
	this->kd = Vector3(0.3, 0.3, 0.3);
	this->ks = Vector3(0.3, 0.3, 0.3);
	this->alpha_sh = 2;

	this->ambient_light = Vector3(0.3, 0.3, 0.3);
	this->light_diffuse = Vector3(0.3, 0.3, 0.3);
	this->light_specular = Vector3(0.7, 0.7, 0.7);

	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/volume.fs");

}

void VolumeMaterial::setUniforms(Camera* camera, Matrix44 model) {
	StandardMaterial::setUniforms(camera, model);

	shader->setUniform("u_volume_texture", volume_texture, (int)TextureSlots::VOLUME);
	shader->setUniform("u_step_length", this->step_length);
	shader->setUniform("u_brightness", this->brightness);
	shader->setUniform("u_noise_texture", this->noise_texture, (int)TextureSlots::NOISE);
	shader->setUniform("u_noise_texture_width", this->noise_texture->width);
	if (classification_option == ClassificationOption::TF) {
		shader->setUniform("u_tf_texture", this->tf_texture, (int)TextureSlots::TF);
	}
	shader->setUniform("u_classification_option", (float)this->classification_option);
	shader->setUniform("u_plane_parameters", this->plane_parameters);
	shader->setUniform("u_apply_plane", this->apply_plane);
	shader->setUniform("u_isovalue", this->isovalue);
	shader->setUniform("u_h", this->h);
	Matrix44 inv_model = model;
	inv_model.inverse();
	shader->setUniform("u_inv_model", inv_model);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Light PBR params
	shader->setUniform("u_light_pos", this->light_position);
	shader->setUniform("u_light_intensity", this->light_intentsity);
	shader->setUniform("u_light_color", this->light_color);

	// Material PBR props
	shader->setUniform("u_roughness_factor", this->roughness);
	shader->setUniform("u_metalness_factor", this->metalness);

	// Light Phong params
	shader->setUniform("u_ka", ka);
	shader->setUniform("u_kd", kd);
	shader->setUniform("u_ks", ks);
	shader->setUniform("u_alpha_sh", alpha_sh);
	shader->setUniform("u_ambient_light", ambient_light);
	shader->setUniform("u_light_diffuse", light_diffuse);
	shader->setUniform("u_light_specular", light_specular);

}

void VolumeMaterial::loadPresets(bool changed) {
	if (!changed)
		return;

	switch (this->classification_option)
	{
	case ClassificationOption::PART1:
		part1FootPreset();
		break;
	case ClassificationOption::TF:
		tfFootPreset();
		break;
	case ClassificationOption::ISOPBR:
		isoPBRFootPreset();
		break;
	case ClassificationOption::ISOPHONG:
		isoPhongFootPreset();
		break;
	}
}

void VolumeMaterial::loadPresetsPerVolume(bool changed) {
	if (!changed) {
		return;
	}
	switch (this->classification_option)
	{
	case ClassificationOption::PART1:
		part1PresetSelector();
		break;
	case ClassificationOption::TF:
		tfPresetSelector();
		break;
	case ClassificationOption::ISOPBR:
		isoPBRPresetSelector();
		break;
	case ClassificationOption::ISOPHONG:
		isoPhongPresetSelector();
		break;
	}


}


void VolumeMaterial::renderInMenu() {
	bool classi_changed = false;
	classi_changed |= ImGui::Combo("Classification Option", (int*)&this->classification_option, "PART1\0TF\0ISO PBR\0ISO PHONG\0");
	// Load presets
	loadPresets(classi_changed);

	bool volume_changed = false;
	volume_changed |= ImGui::Combo("Sphere texture", (int*)&this->textures_volume_index, "FOOT\0TEA POT\0BONSAI\0");
	loadPresetsPerVolume(volume_changed);

	ImGui::SliderFloat("Step length", &this->step_length, 0.01f, 1.0f);
	ImGui::SliderFloat("Brightness", &this->brightness, 0.0f, 5.0f);

	ImGui::Checkbox("Plane", &this->apply_plane);
	if (this->apply_plane)
		ImGui::SliderFloat4("Plane parameters", (float*)&this->plane_parameters, -1.0, 1.0);

	if (this->classification_option == ClassificationOption::ISOPBR || this->classification_option == ClassificationOption::ISOPHONG) {
		if (ImGui::TreeNode("Isosurface")) {
		ImGui::SliderFloat("Isovalue", (float*)&this->isovalue, 0.01f, 1.0f);
		ImGui::SliderFloat("h step", &this->h, 0.001, 0.2);
		ImGui::TreePop();
		}
		if (ImGui::TreeNode("Light params")) {
			// Light params
			ImGui::SliderFloat3("Light pos", (float*)&this->light_position, -15.0, 15.0);
			ImGui::SliderFloat("Light Inten", (float*)&this->light_intentsity, 0.0, 10.0);
			ImGui::ColorEdit3("Light Color", (float*)&this->light_color);
			ImGui::TreePop();

		}

		if (this->classification_option == ClassificationOption::ISOPBR) {
			if (ImGui::TreeNode("PBR params")) {
				// Material params
				ImGui::SliderFloat("Roughness", &this->roughness, 0.01, 1.0);
				ImGui::SliderFloat("Metalness", &this->metalness, 0.01, 1.0);
				ImGui::TreePop();

			}
		}
		else {
			if (ImGui::TreeNode("Phong params")) {
				ImGui::DragFloat3("Ambient reflection (ka)", (float*)&this->ka, 0.005f, 0.0f, 1.0f);
				ImGui::DragFloat3("Diffuse reflection (kd)", (float*)&this->kd, 0.005f, 0.0f, 1.0f);
				ImGui::DragFloat3("Specular reflection (ks)", (float*)&this->ks, 0.005f, 0.0f, 1.0f);
				ImGui::SliderFloat("Alpha", &this->alpha_sh, 0.01, 10);
				ImGui::ColorEdit3("Ambient light", (float*)&ambient_light);
				ImGui::ColorEdit3("Diffuse light", (float*)&light_diffuse);
				ImGui::ColorEdit3("Specular light", (float*)&light_specular);
				ImGui::TreePop();

			}
		}
	}

	
}


void VolumeMaterial::volumeUpdate() {
	if ((int)textures_volume_index >= std::size(volume_textures)) {
		return;
	}
	volume_texture = volume_textures[(int)textures_volume_index];
	tf_texture = tf_textures[(int)textures_volume_index];
}

// Presets per volume
// Part 1
void VolumeMaterial::part1PresetSelector() {
	switch (this->textures_volume_index)
	{
	case VolumeOption::FOOT:
		part1FootPreset();
		break;
	case VolumeOption::TEAPOT:
		part1TeapotPreset();
		break;
	case VolumeOption::BONSAI:
		part1BonsaiPreset();
		break;
	}
}
void VolumeMaterial::part1FootPreset() {
	this->step_length = 0.048;
	this->brightness = 0.6;
	this->textures_volume_index = VolumeOption::FOOT;
	volumeUpdate();
}
void VolumeMaterial::part1TeapotPreset() {
	this->step_length = 0.032;
	this->brightness = 0.881;
	this->textures_volume_index = VolumeOption::TEAPOT;
	volumeUpdate();
}
void VolumeMaterial::part1BonsaiPreset(){
	this->step_length = 0.032;
	this->brightness = 0.683;
	this->textures_volume_index = VolumeOption::BONSAI;
	volumeUpdate();
}
// Transfer functions
void VolumeMaterial::tfPresetSelector() {
	switch (this->textures_volume_index)
	{
	case VolumeOption::FOOT:
		tfFootPreset();
		break;
	case VolumeOption::TEAPOT:
		tfTeapotPreset();
		break;
	case VolumeOption::BONSAI:
		tfBonsaiPreset();
		break;
	}
}
void VolumeMaterial::tfFootPreset() {
	this->step_length = 0.048;
	this->brightness = 0.6;
	//Choose transfer function
	//...
	this->textures_volume_index = VolumeOption::FOOT;
	volumeUpdate();
}
void VolumeMaterial::tfTeapotPreset() {
	this->step_length = 0.032;
	this->brightness = 0.881;
	//Choose transfer function
	//...
	this->textures_volume_index = VolumeOption::TEAPOT;
	volumeUpdate();
}
void VolumeMaterial::tfBonsaiPreset() {
	this->step_length = 0.032;
	this->brightness = 0.683;
	//Choose transfer function
	//...
	this->textures_volume_index = VolumeOption::BONSAI;
	volumeUpdate();
}
// Isosurface PBR
void VolumeMaterial::isoPBRPresetSelector() {
	switch (this->textures_volume_index)
	{
	case VolumeOption::FOOT:
		isoPBRFootPreset();
		break;
	case VolumeOption::TEAPOT:
		isoPBRTeapotPreset();
		break;
	case VolumeOption::BONSAI:
		isoPBRBonsaiPreset();
		break;
	}
}
void VolumeMaterial::isoPBRFootPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.311;
	// GRadient step estimator
	this->h = 0.05;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);
	// PBR params
	this->roughness = 0.612;
	this->metalness = 0.868;

	// Volume params
	this->textures_volume_index = VolumeOption::FOOT;
	volumeUpdate();
}
void VolumeMaterial::isoPBRTeapotPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.185;
	// GRadient step estimator
	this->h = 0.005;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);
	// PBR params
	this->roughness = 0.503;
	this->metalness = 0.938;

	// Volume params
	this->textures_volume_index = VolumeOption::TEAPOT;
	volumeUpdate();
}
void VolumeMaterial::isoPBRBonsaiPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.224;
	// GRadient step estimator
	this->h = 0.005;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);
	// PBR params
	this->roughness = 0.546;
	this->metalness = 0.748;

	// Volume params
	this->textures_volume_index = VolumeOption::BONSAI;
	volumeUpdate();
}

// Isosurface Phong
void VolumeMaterial::isoPhongPresetSelector() {
	switch (this->textures_volume_index)
	{
	case VolumeOption::FOOT:
		isoPhongFootPreset();
		break;
	case VolumeOption::TEAPOT:
		isoPhongTeapotPreset();
		break;
	case VolumeOption::BONSAI:
		isoPhongBonsaiPreset();
		break;
	}
}
void VolumeMaterial::isoPhongFootPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.307;
	// GRadient step estimator
	this->h = 0.013;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);

	// Phong params
	this->ka = Vector3(0.16, 0.21, 0.035);
	this->kd = Vector3(0.11, 0.05, 0.02);
	this->ks = Vector3(0.705, 0.7, 0.7);
	this->alpha_sh = 1.5;
	this->ambient_light = Vector3(0.56, 0.43, 1.0);
	this->light_diffuse = Vector3(0.56, 0.43, 1.0);
	this->light_specular = Vector3(0.56, 0.43, 1.0);
	// Volume params
	this->textures_volume_index = VolumeOption::FOOT;
	volumeUpdate();
}
void VolumeMaterial::isoPhongTeapotPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.158;
	// GRadient step estimator
	this->h = 0.006;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);

	// Phong params
	this->ka = Vector3(0.16, 0.21, 0.035);
	this->kd = Vector3(0.11, 0.05, 0.02);
	this->ks = Vector3(0.705, 0.7, 0.7);
	this->alpha_sh = 1.5;
	this->ambient_light = Vector3(0.56, 0.43, 1.0);
	this->light_diffuse = Vector3(0.56, 0.43, 1.0);
	this->light_specular = Vector3(0.56, 0.43, 1.0);
	// Volume params
	this->textures_volume_index = VolumeOption::TEAPOT;
	volumeUpdate();
}
void VolumeMaterial::isoPhongBonsaiPreset() {
	// Ray params
	this->step_length = 0.01;
	this->brightness = 0.917;
	// Isosurface
	this->isovalue = 0.154;
	// GRadient step estimator
	this->h = 0.005;
	// Light params
	this->light_position = Vector3(-10.0, 9.5, 6.29);
	this->light_intentsity = 2.5;
	this->light_color = Vector4(0.56, 0.43, 1.0, 1.0);

	// Phong params
	this->ka = Vector3(0.16, 0.21, 0.035);
	this->kd = Vector3(0.11, 0.05, 0.02);
	this->ks = Vector3(0.705, 0.7, 0.7);
	this->alpha_sh = 1.5;
	this->ambient_light = Vector3(0.56, 0.43, 1.0);
	this->light_diffuse = Vector3(0.56, 0.43, 1.0);
	this->light_specular = Vector3(0.56, 0.43, 1.0);
	// Volume params
	this->textures_volume_index = VolumeOption::BONSAI;
	volumeUpdate();
}