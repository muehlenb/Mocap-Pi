#pragma once

#include <vector>
#include <string>

struct GuiUtils {
    /**
     * Converts a vector<string> to a string where elements are separated by \0
     */
    static std::string VectorToString(const std::vector<std::string>& vec) {
        std::string result;
        for (const auto& str : vec) {
            result += str + '\0';
        }
        return result;
    }
};
