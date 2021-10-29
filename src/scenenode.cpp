#include "scenenode.h"
#include "application.h"
#include "texture.h"
#include "utils.h"

unsigned int SceneNode::lastNameId = 0;
unsigned int mesh_selected = 0;

SceneNode::SceneNode()
{
	this->name = std::string("Node" + std::to_string(lastNameId++));
}


SceneNode::SceneNode(const char * name)
{
	this->name = name;
}

SceneNode::~SceneNode()
{

}

void SceneNode::render(Camera* camera)
{
	if (material)
		material->render(mesh, model, camera);
}

void SceneNode::renderWireframe(Camera* camera)
{
	WireframeMaterial mat = WireframeMaterial();
	mat.render(mesh, model, camera);
}

void SceneNode::renderInMenu()
{
	//Model edit
	if (ImGui::TreeNode("Model")) 
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(model.m, matrixTranslation, matrixRotation, matrixScale);
		ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
		ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
		ImGui::DragFloat3("Scale", matrixScale, 0.1f);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, model.m);
		
		ImGui::TreePop();
	}

	//Material
	if (material && ImGui::TreeNode("Material"))
	{
		material->renderInMenu();
		ImGui::TreePop();
	}

	//Geometry
	if (mesh && ImGui::TreeNode("Geometry"))
	{
		bool changed = false;
		changed |= ImGui::Combo("Mesh", (int*)&mesh_selected, "SPHERE\0HELMET\0");

		ImGui::TreePop();
	}
}

ObjectNode::ObjectNode(const char* name) {
	type = SceneNodeTypes::OBJECT;
	this->name = name;
}

PBRNode::PBRNode(const char* name) {
	type = SceneNodeTypes::PBRNODE;
	this->name = name;
}

void PBRNode::render(Camera* camera, Light* light) {
	light->setUniforms();
	SceneNode::render(camera);
}

Light::Light(Vector3 position, Vector4 color, Vector3 intensity, const char* name) {
	this->name = name;
	type = SceneNodeTypes::LIGHT;
	this->model.setTranslation(position.x, position.y, position.z);
	this->color = color;
	this->intensity = intensity;	
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/pbr.fs"); // CANVIAR SHADER
}

void Light::setUniforms() {
	shader->enable();
	shader->setUniform("u_light_pos", model.getTranslation());
	shader->setUniform("u_light_color", color);
	shader->setUniform("u_light_intensity", intensity);
	//shader->setUniform("u_ambient_light", Application::instance->ambient_light);
	shader->disable();
}

void Light::renderInMenu() {
	SceneNode::renderInMenu();
	if (ImGui::TreeNode("Light Attributes")) {
		ImGui::ColorEdit3("Color Light", (float*)&this->color);
		ImGui::DragFloat3("Intensity light",(float*)&this->intensity, 0.005f, 0.0f, 1.0f);
		ImGui::TreePop();
	}
}

SkyboxNode::SkyboxNode(const char* name) {
	type = SceneNodeTypes::SKYBOX;
	this->name = name;
}

void SkyboxNode::syncCameraPosition(Vector3 eye) {
	this->model.setTranslation(eye.x, eye.y, eye.z);
}

void SkyboxNode::render(Camera* camera) {
	glDisable(GL_DEPTH_TEST);

	SceneNode::render(camera);

	glEnable(GL_DEPTH_TEST);
}

/*
Environment::Environment(const char* name) {
	type = SceneNodeTypes::ENVIRONMENT;
	this->name = name;
}

void Environment::setUniforms() {

	// pasar el array con todas las blurred versions of HDRE (hdre_versions[] defined in application) NO SE COM HACERLO 

	shader->enable();
	shader->setUniform("u_texture_prem_0", hdre_versions[0]);
	shader->setUniform("u_texture_prem_1", hdre_versions[1]);
	shader->setUniform("u_texture_prem_2", hdre_versions[2]);
	shader->setUniform("u_texture_prem_3", hdre_versions[3]);
	shader->setUniform("u_texture_prem_4", hdre_versions[4]);
	shader->disable();
}
*/