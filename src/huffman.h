#pragma once
#include <vector>
#include <string>
#include <memory> // for std::unique_ptr

namespace Cmp {
    // Huffman木を構成するノード
    struct HuffmanNode {
        char data; // 葉ノードの場合の文字
        int freq;  // 出現頻度
        std::unique_ptr<HuffmanNode> left = nullptr;
        std::unique_ptr<HuffmanNode> right = nullptr;

        // 優先度付きキューで比較するための演算子
        bool operator>(const HuffmanNode& other) const {
            return freq > other.freq;
        }
    };

    class Huffman {
    public:
        // データをHuffman符号化で圧縮する
        static std::vector<char> Compress(const std::vector<char>& data);
        // データをHuffman符号化から解凍する
        static std::vector<char> Decompress(const std::vector<char>& data);
    };
}