#pragma once
#include <vector>

namespace Cmp {
    class ArithmeticCoder {
    public:
        static std::vector<char> Compress(const std::vector<char>& data);
        static std::vector<char> Decompress(const std::vector<char>& data);
    };
}