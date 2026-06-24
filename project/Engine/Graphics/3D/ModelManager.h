#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include "Model.h"
#include "Object3dCommon.h"

class ModelManager {
public:
    static void Initialize(Object3dCommon* common);
    static void Finalize();

    static Model* Load(const std::string& modelName);
    static Model* Load(const std::string& directoryPath, const std::string& modelName);
    static Model* Find(const std::string& modelName);

private:
    static Object3dCommon* common_;
    static std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};