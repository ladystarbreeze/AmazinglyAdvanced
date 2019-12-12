#pragma once
#ifndef AMAZINGLY_ADVANCED_FILE_UTILS_H
#define AMAZINGLY_ADVANCED_FILE_UTILS_H


#include "log.h"

#include <cinttypes>
#include <fstream>
#include <iterator>
#include <vector>

inline std::vector<uint8_t> load_file(const char *const file_path, const bool check_size = false, const size_t size = 0)
{
    spdlog::get("AmazinglyAdvanced")->info("Loading file {}... ", file_path);

    std::ifstream  file(file_path, std::ios::binary);
    std::streampos file_size;
    std::vector<uint8_t> data;

    if (!file.is_open())
    {
        spdlog::get("AmazinglyAdvanced")->error("Couldn't loaded file {}!", file_path);

        throw std::runtime_error("Error loading file!");
    }

    file.unsetf(std::ios::skipws);
    file.seekg(std::ios::end);

    file_size = file.tellg();

    file.seekg(std::ios::beg);
    data.resize(file_size);
    data.insert(data.begin(), std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());

    size_t vec_size = data.size() - 2u;

    if (check_size && vec_size != size)
    {
        spdlog::get("AmazinglyAdvanced")->error("File size is {}, expected size is {}!", vec_size, size);

        throw std::runtime_error("File size doesn't match expected size!");
    }

    return data;
}


#endif //AMAZINGLY_ADVANCED_FILE_UTILS_H
