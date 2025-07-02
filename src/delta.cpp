#include "delta.h"

namespace Cmp {
    std::vector<char> Delta::Compress(const std::vector<char>& data, int stride) {
        if ( data.size() < stride ) return data;

        std::vector<char> compressedData = data;
        for ( size_t i = data.size() - 1; i >= stride; --i ) {
            compressedData[i] = data[i] - data[i - stride];
        }
        return compressedData;
    }

    std::vector<char> Delta::Decompress(const std::vector<char>& data, int stride) {
        if ( data.size() < stride ) return data;

        std::vector<char> decompressedData = data;
        for ( size_t i = stride; i < data.size(); ++i ) {
            decompressedData[i] = decompressedData[i] + decompressedData[i - stride];
        }
        return decompressedData;
    }
}