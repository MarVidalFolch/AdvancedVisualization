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
	std::vector<char*> filenames_texture = { "data/blueNoise.png", "data/brdfLUT.png", "data/alex_mar.png" };
	int filename_index_texture;
	
	TextureMaterial(Texture* texture = NULL);
	TextureMaterial(char* filename_texture, Texture* texture = NULL);
	void renderInMenu();
	void textureUpdate();
};

class PhongMaterial : public TextureMaterial{
public:
	Vector3 ka;
	Vector3 kd;
	Vector3 ks;
	float alpha_sh;

	PhongMaterial(char* filename_texture, Vector4 color, Vector3 ka, Vector3 kd, Vector3 ks, float alpha_sh, Shader* shader = NULL, Texture* texture = NULL);
	void setUniforms(Camera* camera, Matrix44 model);
	void renderInMenu();

};

class SkyboxMaterial : public TextureMaterial {
public:
	std::vector<char*> folder_names = { "data/environments/pisa.hdre", "data/environments/panorama.hdre", "data/environments/studio.hdre"};
	int folder_index;
	std::vector<Texture*> hdre_versions; // We save a pointer to the vectors, although we are only interested in the first position


	SkyboxMaterial(char* folder_texture, std::vector<Texture*> hdre_versions, Texture* texture = NULL, Shader* shader = NULL);
	void renderInMenu();
	void textureSkyboxUpdate();
};

class ReflectionMaterial : public StandardMaterial {
public:
	static SkyboxMaterial* skybox;
	ReflectionMaterial(SkyboxMaterial* skybox);
	void setUniforms(Camera* camera, Matrix44 model);
};


class PBRMaterial : public TextureMaterial {
public:
	Texture* roughness_texture;
	Texture* metalness_texture;
	Texture* albedo_texture;
	Texture* normal_texture;
	Texture* ambient_occlusion_texture;
	Texture* oppacity_texture;
	bool is_ao_texture; // Ambient occlusion flag
	bool is_op_texture; // Oppacity map flag
	float roughness_factor;
	float metalness_factor;

	Texture* brdfLUT_texture;
	std::vector<Texture*> hdre_versions_environment;

	PBRMaterial(char* filename_texture, Texture* texture = NULL);
	PBRMaterial(float roughness_factor, float metalness_factor);
	//void renderInMenu();
	void setUniforms(Camera* camera, Matrix44 model);
	void renderInMenu();
};




#endif