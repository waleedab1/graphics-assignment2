#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

struct Material {
	glm::vec4 Albedo = glm::vec4(1.0f);
	glm::vec4 Diffuse = glm::vec4(1.0f);
	glm::vec4 Specular = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
	float Reflective = 0.0f;
	float Transparency = 0.0f;
	float Exponent = 0.0f;
};

struct Sphere {
	glm::vec3 Position = glm::vec3(0.0f);
	float Radius = 0.5f;
	int MaterialIndex = 0;
};

struct Plane {
	glm::vec3 PlaneNormal = glm::vec3(0.0f);
	float d = -1.0f;
	int MaterialIndex = 0;
};

enum ObjectType { PLANE, SPHERE };
enum LightType { DIRECTIONAL, SPOTLIGHT };

struct Light {
	glm::vec3 Direction = glm::vec3(0.0f);
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec4 Intensity = glm::vec4(0.0f);
	float Cutoff = 0;
	int type = 0;
};

struct CustomCamera {
	glm::vec3 Position = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 Target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct CustomScene {
	glm::vec4 Ambient = glm::vec4(1.0f);
	float SphereRefractiveIndex = 1.5f;
	float AirRefractiveIndex = 1.0f;
	bool AntiAliasing = true;

	CustomCamera camera;

	std::vector<Sphere> Spheres;
	std::vector<Plane> Planes;
	std::vector<Light> Lights;
	std::vector<Material> Materials;
};