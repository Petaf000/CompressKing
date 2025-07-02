#pragma once
#include <vector>
#include <string>
#include <memory> // for std::unique_ptr

namespace Cmp {
    // Huffman�؂��\������m�[�h
    struct HuffmanNode {
        char data; // �t�m�[�h�̏ꍇ�̕���
        int freq;  // �o���p�x
        std::unique_ptr<HuffmanNode> left = nullptr;
        std::unique_ptr<HuffmanNode> right = nullptr;

        // �D��x�t���L���[�Ŕ�r���邽�߂̉��Z�q
        bool operator>(const HuffmanNode& other) const {
            return freq > other.freq;
        }
    };

    class Huffman {
    public:
        // �f�[�^��Huffman�������ň��k����
        static std::vector<char> Compress(const std::vector<char>& data);
        // �f�[�^��Huffman����������𓀂���
        static std::vector<char> Decompress(const std::vector<char>& data);
    };
}