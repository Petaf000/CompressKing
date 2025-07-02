#include "rle.h"

namespace Cmp {
    // RLEの特殊マーカーと最大ラン長
    namespace {
        constexpr char RLE_MARKER = (char)0xFE;
        constexpr int MAX_RUN_LENGTH = 255;
    }

    std::vector<char> Rle::Compress(const std::vector<char>& data) {
        std::vector<char> compressedData;
        if ( data.empty() ) return compressedData;

        for ( size_t i = 0; i < data.size(); ++i ) {
            char currentChar = data[i];
            size_t runLength = 1;
            while ( i + 1 < data.size() && data[i + 1] == currentChar && runLength < MAX_RUN_LENGTH ) {
                runLength++;
                i++;
            }

            if ( runLength >= 3 || currentChar == RLE_MARKER ) {
                compressedData.push_back(RLE_MARKER);
                compressedData.push_back(static_cast<char>( runLength ));
                compressedData.push_back(currentChar);
            }
            else {
                for ( size_t k = 0; k < runLength; ++k ) {
                    compressedData.push_back(currentChar);
                }
            }
        }
        return compressedData;
    }

    std::vector<char> Rle::Decompress(const std::vector<char>& data) {
        std::vector<char> decompressedData;
        for ( size_t i = 0; i < data.size(); ++i ) {
            if ( data[i] == RLE_MARKER ) {
                uint8_t runLength = static_cast<uint8_t>( data[i + 1] );
                char D = data[i + 2];
                for ( int j = 0; j < runLength; ++j ) {
                    decompressedData.push_back(D);
                }
                i += 2;
            }
            else {
                decompressedData.push_back(data[i]);
            }
        }
        return decompressedData;
    }
}