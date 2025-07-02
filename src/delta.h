#pragma once
#include <vector>

namespace Cmp {
    class Delta {
    public:
        // stride�͍��������Ԋu�B�����`�����l�����l������B
        static std::vector<char> Compress(const std::vector<char>& data, int stride = 4);
        static std::vector<char> Decompress(const std::vector<char>& data, int stride = 4);
    };
}