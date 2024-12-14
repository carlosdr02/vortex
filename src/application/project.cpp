#include "project.h"

#include <fstream>

Project::Project(const std::filesystem::path& path) : path(path) {
    std::filesystem::create_directory(path);
    std::ofstream projectFile(path / "project.vx");
    std::filesystem::path contentPath = getContentPath();
    std::filesystem::create_directory(contentPath);
    std::filesystem::create_directory(contentPath / "Images");
}

std::filesystem::path Project::getContentPath() {
    return path / "content";
}
