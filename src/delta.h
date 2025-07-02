#pragma once
#include <vector>

namespace Cmp {
    class Delta {
    public:
        // strideは差分を取る間隔。音声チャンネルを考慮する。
        static std::vector<char> Compress(const std::vector<char>& data, int stride = 4);
        static std::vector<char> Decompress(const std::vector<char>& data, int stride = 4);
    };
}