#include "project.h"

#include <filesystem>

static const char* tempProject = "temp_project";

Project::Project() {
    std::filesystem::create_directory(tempProject);

    for (const auto& entry : std::filesystem::directory_iterator(tempProject)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string());
        }
    }
}

void Project::import(const std::string& file) {
    std::filesystem::path source(file);

    try {
        std::filesystem::copy(source, tempProject);
    } catch (const std::filesystem::filesystem_error& e) {
        // TODO:
    }

    files.push_back(source.filename().string());
}
