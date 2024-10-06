#include "project.h"

static const char* tempProject = "temp_project";

Project::Project() {
    std::filesystem::create_directory(tempProject);

    for (const auto& entry : std::filesystem::directory_iterator(tempProject)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
}

void Project::import(const std::filesystem::path& path) {
    try {
        std::filesystem::copy(path, tempProject);
    } catch (const std::filesystem::filesystem_error& e) {
        // TODO:
    }

    files.push_back(path);
}
