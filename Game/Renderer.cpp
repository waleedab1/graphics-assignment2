#include "Renderer.h"
#include <cmath>
#include <glm/glm.hpp> // Assuming you're using GLM for vector math
#include <glm/gtc/random.hpp>
#include "Ray.h"
#include <algorithm>
#include <execution>
#include <iostream>


namespace utils {
	// Function to convert RGBA color components to 32-bit hexadecimal color
	uint32_t rgbaToHex(glm::vec4 color) {
		// Clamp color values to range [0, 1]
		float r = (color.r < 0.0f) ? 0.0f : (color.r > 1.0f) ? 1.0f : color.r;
		float g = (color.g < 0.0f) ? 0.0f : (color.g > 1.0f) ? 1.0f : color.g;
		float b = (color.b < 0.0f) ? 0.0f : (color.b > 1.0f) ? 1.0f : color.b;
		float a = (color.a < 0.0f) ? 0.0f : (color.a > 1.0f) ? 1.0f : color.a;

		// Convert float color components to integer values in range [0, 255]
		uint8_t red = (uint8_t)(r * 255);
		uint8_t green = (uint8_t)(g * 255);
		uint8_t blue = (uint8_t)(b * 255);
		uint8_t alpha = (uint8_t)(a * 255);

		// Pack RGBA values into a single 32-bit integer
		return (alpha << 24) | (blue << 16) | (green << 8) | red;
	}
}

void Renderer::Render(const CustomScene& scene, const CustomCamera& camera) {
	activeScene = &scene;
	activeCamera = &camera;
	int samples = 20;
	int depth = 5;

	std::vector<uint32_t> horizontalIter;
	horizontalIter.resize(height);
	for (uint32_t i = 0; i < height; i++) {
		horizontalIter[i] = i;
	}

	std::for_each(std::execution::par, horizontalIter.begin(), horizontalIter.end(), [this, samples, depth](uint32_t y) {
		for (uint32_t x = 0; x < width; x++) {
			Ray ray = ConstructRayThroughPixel(x, y);
			glm::vec4 accumaltedColor = PerPixel(ray, depth);
			if (activeScene->AntiAliasing) {
				for (int i = 1; i < samples; i++) {
					glm::vec4 color = PerPixel(ray, depth);
					accumaltedColor += color;
				}
				accumaltedColor /= (float)samples;
			}
			image_data[x + y * width] = utils::rgbaToHex(accumaltedColor);
		}
		});
}

Ray Renderer::ConstructRayThroughPixel(uint32_t x, uint32_t y) {
	Ray ray;
	glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
	coord = coord * 2.0f - 1.0f;
	ray.Direction = glm::vec3(coord, 0.0f) - activeCamera->Position;
	ray.Origin = activeCamera->Position;
	return ray;
}

glm::vec4 Renderer::PerPixel(Ray ray, int depth)
{
	if (depth <= 0)
		return glm::vec4(0.0f);

	Renderer::HitPayload payload = TraceRay(ray);

	if (payload.hitDistance < 0.0f) {
		return glm::vec4(0.0f);
	}

	Material material;
	glm::vec4 ObjectColor;

	if (payload.ObjectType == SPHERE) {
		const Sphere& closestSphere = activeScene->Spheres[payload.ObjectIndex];
		material = activeScene->Materials[closestSphere.MaterialIndex];
		ObjectColor = material.Albedo;
	}
	else {
		const Plane& closestPlane = activeScene->Planes[payload.ObjectIndex];
		material = activeScene->Materials[closestPlane.MaterialIndex];
		ObjectColor = getCheckboard(payload.WorldPosition, material.Albedo);
	}

	glm::vec4 I = ObjectColor * activeScene->Ambient;

	if (material.Transparency > 0.0f || material.Reflective > 0.0f)
		I = glm::vec4(0.0f);

	for (size_t i = 0; i < activeScene->Lights.size() && material.Reflective <= 0.0f && material.Transparency <= 0.0f; i++) {
		Light light = activeScene->Lights[i];
		glm::vec3 lightDirection;
		glm::vec3 normal = glm::normalize(payload.WorldNormal);
		float distanceToLight = std::numeric_limits<float>::max();;

		if (light.type == DIRECTIONAL) {
			lightDirection = -glm::normalize(light.Direction);
		}
		else if (light.type == SPOTLIGHT) {
			lightDirection = glm::normalize(light.Position - payload.WorldPosition);
			if (!IsInSpotLight(lightDirection, glm::normalize(light.Direction), light.Cutoff))
				continue;
			distanceToLight = glm::distance(light.Position, payload.WorldPosition);
		}

		// need to check if from intersection to light is not blocked.
		Ray toLight;
		toLight.Origin = payload.WorldPosition + 0.001f*normal;
		toLight.Direction = lightDirection;
		Renderer::HitPayload r = TraceRay(toLight);
		if (r.hitDistance >= 0.0f && r.hitDistance < distanceToLight) {
			continue;
		}

		// Calculate light color.
		glm::vec4 diffuseCalculation = CalculateDiffuse(lightDirection, normal, ObjectColor);
		glm::vec4 specularCalculation = CalculateSpecular(ray.Direction, lightDirection, normal, material.Specular, material.Exponent);
		I += (diffuseCalculation + specularCalculation) * light.Intensity;
	}

	// Reflective surfaces.
	if (material.Reflective > 0.0f) {
		Ray secondaryRay;
		secondaryRay.Origin = payload.WorldPosition + 0.001f * glm::normalize(payload.WorldNormal);
		secondaryRay.Direction = glm::reflect(ray.Direction, glm::normalize(payload.WorldNormal));

		I += material.Specular * PerPixel(secondaryRay, depth - 1);
	}

	// Transparent surfaces.
	if (material.Transparency > 0.0f) {
		Ray secondaryRay;
		secondaryRay.Origin = payload.WorldPosition - 0.001f * payload.WorldNormal;
		if (!ray.InsideSphere) {
			secondaryRay.Direction = Refract(ray.Direction, payload.WorldNormal, activeScene->AirRefractiveIndex, activeScene->SphereRefractiveIndex);
			secondaryRay.InsideSphere = true;
		}
		else {
			secondaryRay.Direction = Refract(ray.Direction, payload.WorldNormal, activeScene->SphereRefractiveIndex, activeScene->AirRefractiveIndex);
		}

		I += PerPixel(secondaryRay, depth - 1) * material.Transparency;
	}

	return I;
}

bool Renderer::IsInSpotLight(glm::vec3 directionToLight, glm::vec3 lightDirection, float cutoff) {
	float cosAngle = glm::dot(glm::normalize(-lightDirection), directionToLight);
	return !(cosAngle < cutoff);
}

glm::vec4 Renderer::CalculateDiffuse(glm::vec3 lightDirection, glm::vec3 normal, glm::vec4 diffuseColor) {
	glm::vec4 diffuseCalculation = diffuseColor * (glm::dot(normal, lightDirection));
	return diffuseCalculation;
}

glm::vec4 Renderer::CalculateSpecular(glm::vec3 toViewer, glm::vec3 lightDirection, glm::vec3 normal, glm::vec4 specularColor, float exponent) {
	glm::vec3 V = glm::normalize(toViewer);
	glm::vec3 R = glm::normalize(glm::reflect(lightDirection, normal));
	float VR = glm::max(glm::dot(V, R), 0.0f);
	glm::vec4 specularCalculation = specularColor * glm::pow(VR, exponent);
	return specularCalculation;
}

glm::vec3 Renderer::Refract(glm::vec3 incidentDirection, glm::vec3 normal, float n1, float n2) {
	incidentDirection = -glm::normalize(incidentDirection);
	normal = glm::normalize(normal);

	float n = n1 / n2 - 0.15f; // TO-DO: check why it needs the offset to be accurate

	float cosThetaI = glm::dot(incidentDirection, normal);
	float sinThetaR = n * sqrtf(fmax(0.0f, 1.0f - cosThetaI * cosThetaI));

	// Check for total internal reflection
	if (sinThetaR >= 1.0f) {
		return glm::vec3(0.0f);
	}

	float cosThetaR = sqrtf(fmax(0.0f, 1.0f - sinThetaR * sinThetaR));
	return (n * cosThetaI - cosThetaR) * normal - n * incidentDirection;
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray) {

	// If inside sphere: take the other farthest t

	int closestSphere = -1;
	float h1 = std::numeric_limits<float>::max();
	float far = -1.0f;

	for (size_t i = 0; i < activeScene->Spheres.size(); i++) {
		const Sphere& sphere = activeScene->Spheres[int(i)];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c;

		if (discriminant < 1e-6f)
			continue;

		float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a);
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);

		if (closestT >= 0.0f && closestT < h1) {
			h1 = closestT;
			closestSphere = int(i);
		}
	}

	int closestPlane = -1;
	float h2 = std::numeric_limits<float>::max();

	for (size_t i = 0; i < activeScene->Planes.size(); i++) {
		const Plane& plane = activeScene->Planes[int(i)];

		float denominator = glm::dot(ray.Direction, plane.PlaneNormal);

		if (fabs(denominator) < 1e-6f)
			continue;

		float t = -(glm::dot(plane.PlaneNormal, ray.Origin) + plane.d) / denominator;

		if (t >= 0.0f && t < h2) {
			h2 = t;
			closestPlane = int(i);
		}
	}

	if (closestPlane < 0 && closestSphere < 0)
		return Miss(ray);

	if (closestSphere < 0 || h2 < h1)
		return ClosestHit(ray, h2, closestPlane, PLANE);

	return ClosestHit(ray, h1, closestSphere, SPHERE);
}



Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int ObjectIndex, int type)
{
	Renderer::HitPayload payload;
	payload.hitDistance = hitDistance;
	payload.ObjectIndex = ObjectIndex;
	payload.ObjectType = type;

	if (type == SPHERE) {
		const Sphere& closestSphere = activeScene->Spheres[ObjectIndex];

		glm::vec3 origin = ray.Origin - closestSphere.Position;
		payload.WorldPosition = origin + ray.Direction * hitDistance;
		payload.WorldNormal = glm::normalize(payload.WorldPosition);

		payload.WorldPosition += closestSphere.Position;
	}
	else {
		const Plane& plane = activeScene->Planes[ObjectIndex];

		// Calculate intersection point
		glm::vec3 intersectionPoint = ray.Origin + ray.Direction * hitDistance;

		// Calculate the normal of the plane
		glm::vec3 planeNormal = -glm::normalize(plane.PlaneNormal);

		// Calculate the world position and normal
		payload.WorldPosition = intersectionPoint;
		payload.WorldNormal = planeNormal;
	}
	return payload;
}


Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.hitDistance = -1.0f;
	return payload;
}

glm::vec4 Renderer::getCheckboard(glm::vec3 hitPoint, glm::vec4 color)
{
	float scale_parameter = 0.5f;
	float checkboard = 0;

	if (hitPoint.x < 0) {
		checkboard += floor((0.5 - hitPoint.x) / scale_parameter);
	}
	else {
		checkboard += floor(hitPoint.x / scale_parameter);
	}
	if (hitPoint.y < 0) {
		checkboard += floor((0.5 - hitPoint.y) / scale_parameter);
	}
	else {
		checkboard += floor(hitPoint.y / scale_parameter);
	}

	checkboard = (checkboard * 0.5) - int(checkboard * 0.5);
	checkboard *= 2;
	if (checkboard > 0.5) {
		return 0.5f * color;
	}
	return color;
}

