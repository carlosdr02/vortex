#include "project.h"

#include <filesystem>

static const char* tempProject = "temp_project";

Project::Project() {
    std::filesystem::create_directory(tempProject);
}

void Project::import(const std::string& file) {
    try {
        std::filesystem::copy(file, tempProject);
    } catch (const std::filesystem::filesystem_error& e) {

    }
}
