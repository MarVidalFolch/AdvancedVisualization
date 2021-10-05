#ifndef MATERIAL_H
#define MATERIAL_H

#include "framework.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"
#include "extra/hdre.h"

class Material {
public:

	Shader* shader = NULL;
	Texture* texture = NULL;
	vec4 color;

	virtual void setUniforms(Camera* camera, Matrix44 model) = 0;
	virtual void render(Mesh* mesh, Matrix44 model, Camera * camera) = 0;
	virtual void renderInMenu() = 0;
};

class StandardMaterial : public Material {
public:

	StandardMaterial();
	~StandardMaterial();

	void setUniforms(Camera* camera, Matrix44 model);
	void render(Mesh* mesh, Matrix44 model, Camera * camera);
	void renderInMenu();
};

class WireframeMaterial : public StandardMaterial {
public:

	WireframeMaterial();
	~WireframeMaterial();

	void render(Mesh* mesh, Matrix44 model, Camera * camera);
};

class TextureMaterial : public StandardMaterial {
public:
	TextureMaterial(Texture* texture = NULL);
};

class PhongMaterial : public TextureMaterial{
public:
	Vector3 ka;
	Vector3 kd;
	Vector3 ks;
	float alpha_sh;

	PhongMaterial(Vector4 color, Vector3 ka, Vector3 kd, Vector3 ks, float alpha_sh, Shader* shader = NULL, Texture* texture = NULL);
	void setUniforms(Camera* camera, Matrix44 model);
	void renderInMenu();

};

class SkyboxMaterial : public TextureMaterial {
public:
	SkyboxMaterial(Texture* texture = NULL, Shader* shader = NULL);
};

class ReflectionMaterial : public StandardMaterial {
public:
	static SkyboxMaterial* skybox;
	ReflectionMaterial(SkyboxMaterial* skybox);
};

#endif