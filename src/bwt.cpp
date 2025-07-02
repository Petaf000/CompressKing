#include "bwt.h"
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

namespace Cmp {
    Bwt::BwtResult Bwt::Transform(const std::vector<char>& data) {
        if ( data.empty() ) return { {}, 0 };
        const size_t n = data.size();
        std::vector<int> suffix_array(n);
        std::iota(suffix_array.begin(), suffix_array.end(), 0);
        std::sort(suffix_array.begin(), suffix_array.end(),
            [ & ] (int a, int b) {
                for ( size_t i = 0; i < n; ++i ) {
                    if ( data[( a + i ) % n] != data[( b + i ) % n] ) {
                        return static_cast<unsigned char>( data[( a + i ) % n] ) < static_cast<unsigned char>( data[( b + i ) % n] );
                    }
                }
                return false;
            });
        std::vector<char> transformed(n);
        size_t primary_index = 0;
        for ( size_t i = 0; i < n; ++i ) {
            transformed[i] = data[( suffix_array[i] + n - 1 ) % n];
            if ( suffix_array[i] == 0 ) {
                primary_index = i;
            }
        }
        return { transformed, primary_index };
    }

    std::vector<char> Bwt::InverseTransform(const BwtResult& bwtResult) {
        const auto& L = bwtResult.first;
        const size_t primary_index = bwtResult.second;
        const size_t n = L.size();
        if ( n == 0 ) return {};

        // ššš ‚±‚±‚ªC³“_ ššš
        std::vector<std::pair<unsigned char, int>> pairs(n);
        for ( size_t i = 0; i < n; ++i ) {
            pairs[i] = { static_cast<unsigned char>( L[i] ), static_cast<int>( i ) };
        }
        std::sort(pairs.begin(), pairs.end());

        std::vector<int> T(n);
        for ( size_t i = 0; i < n; ++i ) {
            T[pairs[i].second] = i;
        }

        std::vector<char> original(n);
        size_t current_index = primary_index;
        for ( int i = n - 1; i >= 0; --i ) {
            original[i] = L[current_index];
            current_index = T[current_index];
        }
        return original;
    }
}