#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace Cmp {
    // Lz77Token �\���͕̂ύX�Ȃ�
    struct Lz77Token {
        uint16_t distance;
        uint8_t length;
        char nextChar;
    };

    class Lz77 {
    public:
        // ���k�E�𓀊֐��͕ύX�Ȃ�
        static std::vector<Lz77Token> Compress(const std::vector<char>& data);
        static std::vector<char> Decompress(const std::vector<Lz77Token>& tokens);

        // ����������ǉ���
        // �g�[�N�����X�g���o�C�g��ɕϊ�����
        static std::vector<char> SerializeTokens(const std::vector<Lz77Token>& tokens);
        // �o�C�g����g�[�N�����X�g�ɕϊ�����
        static std::vector<Lz77Token> DeserializeTokens(const std::vector<char>& data);
    };
}