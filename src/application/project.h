#pragma once

#include <filesystem>

class Project {
public:
    std::filesystem::path path;

    Project() = default;
    Project(const std::filesystem::path& path);

    std::filesystem::path getContentPath();
};
