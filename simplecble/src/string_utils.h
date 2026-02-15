#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

inline char* str_to_c(const std::string& str) {
    char* c_str = static_cast<char*>(malloc(str.size() + 1));
    if (c_str != nullptr) {
        strcpy(c_str, str.c_str());
    }
    return c_str;
}
