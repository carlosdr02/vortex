#include "project.h"

#include <fstream>

Project::Project(const std::filesystem::path& path) : path(path) {
    std::filesystem::create_directory(path);
    std::ofstream projectFile(path / "project.vx");
    std::filesystem::path assetsDirectoryPath = getAssetsDirectoryPath();
    std::filesystem::create_directory(assetsDirectoryPath);
    std::filesystem::create_directory(assetsDirectoryPath / "Images");
    std::filesystem::create_directory(assetsDirectoryPath / "Samplers");
    std::filesystem::create_directory(assetsDirectoryPath / "Shaders");
}

std::filesystem::path Project::getAssetsDirectoryPath() {
    return path / "Assets";
}
