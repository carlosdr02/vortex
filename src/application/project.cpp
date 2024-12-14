#include "project.h"

#include <fstream>

Project::Project(const std::filesystem::path& path) : path(path) {
    std::filesystem::create_directory(path);
    std::filesystem::create_directory(path / "content");
    std::ofstream projectFile(path / "project.vx");
}
