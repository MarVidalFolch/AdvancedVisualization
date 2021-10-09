#ifndef SCENENODE_H
#define SCENENODE_H

#include "framework.h"

#include "shader.h"
#include "mesh.h"
#include "camera.h"
#include "material.h"

class Light;

enum class SceneNodeTypes {
	OBJECT,
	PHONGNODE,
	SKYBOX,
	LIGHT
};

class SceneNode {
public:
	SceneNodeTypes type;
	static unsigned int lastNameId;

	SceneNode();
	SceneNode(const char* name);
	~SceneNode();

	Material * material = NULL;
	std::string name;

	Mesh* mesh = NULL;
	Matrix44 model;

	virtual void render(Camera* camera);
	virtual void renderWireframe(Camera* camera);
	virtual void renderInMenu();
};

class ObjectNode : public SceneNode {
public:
	ObjectNode(const char* name = "OBJECT NODE");

};

class PhongNode : public SceneNode {
public:
	PhongNode(const char* name = "PHONG NODE");

	void render(Camera* camera, Light* light);
};

class Light : public SceneNode {
public:
	Vector4 color;
	Vector3 diffuse;
	Vector3 specular;
	float max_distance;
	Shader* shader;
	Light(Vector3 position, Vector4 color, Vector3 diffuse, Vector3 specular, float max_distance, const char* name = "LIGHT NODE");
	void setUniforms();
	void renderInMenu();
};

class SkyboxNode : public SceneNode {
public:
	SkyboxNode(const char* name = "SKYBOX NODE");

	void syncCameraPosition(Vector3 eye);
	void render(Camera* camera);
};

#endif