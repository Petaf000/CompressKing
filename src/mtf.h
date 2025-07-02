#pragma once
#include <vector>

namespace Cmp {
    class Mtf {
    public:
        static std::vector<char> Transform(const std::vector<char>& data);
        static std::vector<char> InverseTransform(const std::vector<char>& data);
    };
}