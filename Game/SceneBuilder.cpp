#include "SceneBuilder.h"

vector<SceneBuilder::Data> SceneBuilder::ParseFile(const string& path) {
	vector<Data> dataList;

	// Open the file
	ifstream inputFile(path);
	if (!inputFile) {
		cerr << "Error opening file." << endl;
		return dataList; // Return an empty vector if file opening fails
	}

	// Read the file line by line
	string line;
	while (getline(inputFile, line)) {
		stringstream ss(line);
		Data data;
		ss >> data.type >> data.float1 >> data.float2 >> data.float3 >> data.float4;
		dataList.push_back(data);
	}

	// Close the file
	inputFile.close();

	return dataList;
}

CustomScene SceneBuilder::BuildScene()
{
	CustomScene scene;
	//vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\scene1.txt");
	//vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\scene2.txt");
	//vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\scene3.txt");
	//vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\scene4.txt");
	//vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\scene5.txt");
	vector<Data> artifacts = ParseFile("C:\\Users\\1wja1\\OneDrive\\Desktop\\Graphics\\assignment 2\\custom_scene.txt");

	vector<Data> objects;
	vector<Data> colors;
	vector<Data> lights;
	vector<Data> lightPositions;
	vector<Data> lightIntensities;


	for each (Data var in artifacts)
	{
		switch (var.type)
		{
		case 'e':
			scene.camera.Position = glm::vec3(var.float1, var.float2, var.float3);
			// Anti-Aliasing
			scene.AntiAliasing = var.float4 > 0.0f ? true : false;
			break;
		case 'a':
			scene.Ambient = glm::vec4(var.float1, var.float2, var.float3, var.float4);
			break;
		case 'o':
			objects.push_back(var);
			break;
		case 't':
			objects.push_back(var);
			break;
		case 'r':
			objects.push_back(var);
			break;
		case 'c':
			colors.push_back(var);
			break;
		case 'd':
			lights.push_back(var);
			break;
		case 'p':
			lightPositions.push_back(var);
			break;
		case 'i':
			lightIntensities.push_back(var);
			break;
		default:
			cerr << "undefined type of artifact in scene." << endl;
			break;
		}
	}


	for (int i = 0; i < objects.size(); i++) {
		Data object = objects[i];
		if (object.float4 >= 0.0f) {
			Sphere sphere;
			sphere.Position = glm::vec3(object.float1, object.float2, object.float3);
			sphere.Radius = object.float4;
			sphere.MaterialIndex = i;
			scene.Spheres.push_back(sphere);
		}
		else {
			Plane plane;
			plane.PlaneNormal = glm::vec3(object.float1, object.float2, object.float3);
			plane.d = object.float4;
			plane.MaterialIndex = i;
			scene.Planes.push_back(plane);
		}
		Data color = colors[i];
		Material material;
		material.Albedo = glm::vec4(color.float1, color.float2, color.float3, color.float4);
		material.Diffuse = glm::vec4(color.float1, color.float2, color.float3, color.float4);
		material.Exponent = color.float4;
		material.Specular = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
		material.Transparency = object.type == 't' ? 1.0f : 0.0f;
		material.Reflective = object.type == 'r' ? 1.0f : 0.0f;
		scene.Materials.push_back(material);
	}

	int spotLights = 0;
	for (int i = 0; i < lights.size(); i++) {
		Data l = lights[i];
		Light light;
		Data intensity = lightIntensities[i];
		light.Direction = glm::vec3(l.float1, l.float2, l.float3);
		light.Intensity = glm::vec4(intensity.float1, intensity.float2, intensity.float3, intensity.float4);
		light.type = DIRECTIONAL;
		if (l.float4 == 1.0f) {
			Data postion = lightPositions[spotLights];
			light.Position = glm::vec3(postion.float1, postion.float2, postion.float3);
			light.Cutoff = postion.float4;
			spotLights += 1;
			light.type = SPOTLIGHT;
		}
		scene.Lights.push_back(light);
	}

	return scene;
}
