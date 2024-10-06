#pragma once

#include <vector>
#include <filesystem>

class Project {
public:
    std::vector<std::filesystem::path> files;

    Project();

    void import(const std::filesystem::path& path);
};
