#pragma once
#include <vector>
#include <cstdint>

namespace Cmp {
    class Bwt {
    public:
        // first is the transformed data, second is the original index
        using BwtResult = std::pair<std::vector<char>, size_t>;

        static BwtResult Transform(const std::vector<char>& data);
        static std::vector<char> InverseTransform(const BwtResult& bwtResult);
    };
}