#pragma once
#include "CustomScene.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class SceneBuilder {
public:
	CustomScene BuildScene();
private:
    struct Data {
        char type;
        float float1;
        float float2;
        float float3;
        float float4;
    };

    vector<Data> ParseFile(const string& path);
};