#include "project.h"

#include <filesystem>

static const char* tempProject = "temp_project";

Project::Project() {
    std::filesystem::create_directory(tempProject);
}
