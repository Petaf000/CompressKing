#pragma once
#include <vector>

namespace Cmp {
    class ExeFilter {
    public:
        // is_compress‚ªtrue‚È‚ç•ÏŠ·Afalse‚È‚ç‹t•ÏŠ·
        static std::vector<char> Transform(const std::vector<char>& data);
        static std::vector<char> InverseTransform(const std::vector<char>& data);
    };
}