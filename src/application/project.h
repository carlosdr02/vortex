#pragma once

#include <string>
#include <vector>

class Project {
public:
    std::vector<std::string> files;

    Project();

    void import(const std::string& file);
};
