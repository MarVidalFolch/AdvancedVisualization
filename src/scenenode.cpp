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

ObjectNode::ObjectNode() {
	type = SceneNodeTypes::OBJECT;
}

Light::Light(Vector3 position, Vector4 color, Vector3 diffuse, Vector3 specular, float max_distance) {
	type = SceneNodeTypes::LIGHT;
	this->model.setTranslation(position.x, position.y, position.z);
	this->color = color;
	this->diffuse = diffuse;
	this->specular = specular;
	this->max_distance = max_distance;
}

void Light::setUniforms(Shader* shader) {
	shader->setUniform("u_light_pos", model.getTranslation());
	shader->setUniform("u_light_color", color);
	shader->setUniform("u_light_diffuse", diffuse);
	shader->setUniform("u_light_specular", specular);
	shader->setUniform("u_light_max_distance", max_distance);
}

void Light::renderInMenu() {
	if (ImGui::TreeNode("Light Attributes")) {
		ImGui::ColorEdit3("Color Light", (float*)&this->color);
		ImGui::DragFloat3("Diffuse light",(float*)&this->diffuse, 0.05f, 0.0f, 1.0f);
		ImGui::DragFloat3("Specular light", (float*)&this->specular, 0.05f, 0.0f, 1.0f);
		ImGui::TreePop();
	}
}