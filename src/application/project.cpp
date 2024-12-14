#include "project.h"

#include <fstream>

Project::Project(const std::filesystem::path& path) : path(path) {
    std::filesystem::create_directory(path);
    std::ofstream projectFile(path / "project.vx");
    std::filesystem::create_directory(getContentPath());
}

std::filesystem::path Project::getContentPath() {
    return path / "content";
}
