#include "mtf.h"
#include <list>
#include <numeric>
#include <algorithm>

namespace Cmp {
    std::vector<char> Mtf::Transform(const std::vector<char>& data) {
        std::vector<char> transformedData;
        transformedData.reserve(data.size());

        // 0から255までの文字リストを初期化
        std::list<char> alphabet;
        for ( int i = 0; i < 256; ++i ) {
            alphabet.push_back(static_cast<char>( i ));
        }

        for ( char c : data ) {
            int index = 0;
            auto it = alphabet.begin();
            while ( it != alphabet.end() ) {
                if ( *it == c ) {
                    // 1. 文字の現在のインデックスを出力
                    transformedData.push_back(static_cast<char>( index ));

                    // 2. その文字をリストの先頭に移動
                    alphabet.erase(it);
                    alphabet.push_front(c);
                    break;
                }
                it++;
                index++;
            }
        }
        return transformedData;
    }

    std::vector<char> Mtf::InverseTransform(const std::vector<char>& data) {
        std::vector<char> originalData;
        originalData.reserve(data.size());

        std::list<char> alphabet;
        for ( int i = 0; i < 256; ++i ) {
            alphabet.push_back(static_cast<char>( i ));
        }

        for ( char index_char : data ) {
            int index = static_cast<unsigned char>( index_char );

            auto it = alphabet.begin();
            std::advance(it, index);

            // 1. インデックス位置にある文字を取得して出力
            char c = *it;
            originalData.push_back(c);

            // 2. その文字をリストの先頭に移動
            alphabet.erase(it);
            alphabet.push_front(c);
        }
        return originalData;
    }
}