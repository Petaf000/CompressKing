#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace Cmp {
    // Lz77Token 構造体は変更なし
    struct Lz77Token {
        uint16_t distance;
        uint8_t length;
        char nextChar;
    };

    class Lz77 {
    public:
        // 圧縮・解凍関数は変更なし
        static std::vector<Lz77Token> Compress(const std::vector<char>& data);
        static std::vector<char> Decompress(const std::vector<Lz77Token>& tokens);

        // ★ここから追加★
        // トークンリストをバイト列に変換する
        static std::vector<char> SerializeTokens(const std::vector<Lz77Token>& tokens);
        // バイト列をトークンリストに変換する
        static std::vector<Lz77Token> DeserializeTokens(const std::vector<char>& data);
    };
}