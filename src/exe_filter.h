#pragma once
#include <vector>

namespace Cmp {
    class ExeFilter {
    public:
        // is_compress��true�Ȃ�ϊ��Afalse�Ȃ�t�ϊ�
        static std::vector<char> Transform(const std::vector<char>& data);
        static std::vector<char> InverseTransform(const std::vector<char>& data);
    };
}