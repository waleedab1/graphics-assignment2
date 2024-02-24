#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "Ray.h"
#include "CustomScene.h"

class Renderer 
{
public:
	Renderer() {
		image_data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	}

	~Renderer() {
		delete image_data;
	}

	void Render(const CustomScene& scene, const CustomCamera &camera);
	Ray ConstructRayThroughPixel(uint32_t x, uint32_t y);
	uint32_t* GetImage() const { return image_data; };

private:
	struct HitPayload
	{
		float hitDistance = -1.0f;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		int  ObjectIndex = -1;
		int ObjectType = 0;
	};
	glm::vec4 PerPixel(Ray ray, int depth);
	bool IsInSpotLight(glm::vec3 directionToLight, glm::vec3 lightDirection, float cutoff);
	glm::vec4 CalculateDiffuse(glm::vec3 lightDirection, glm::vec3 normal, glm::vec4 diffuseColor);
	glm::vec4 CalculateSpecular(glm::vec3 toViewer, glm::vec3 lightDirection, glm::vec3 normal, glm::vec4 specularColor, float exponent);
	glm::vec3 Refract(glm::vec3 incidentDir, glm::vec3 normal, float n1, float n2);
	HitPayload TraceRay(const Ray& ray);
	HitPayload ClosestHit(const Ray& ray, float hitDistance, int ObjectIndex, int type);
	HitPayload Miss(const Ray& ray);

	glm::vec4 Renderer::getCheckboard(glm::vec3 hitPoint, glm::vec4 color);

private:
	const CustomScene* activeScene = nullptr;
	const CustomCamera* activeCamera = nullptr;

	uint32_t height = 800;
	uint32_t width = 800;
	uint32_t * image_data = nullptr;
};