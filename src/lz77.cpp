#include "lz77.h"
#include <algorithm> // for std::min

namespace Cmp {
    // --- LZ77�p�����[�^ ---
    namespace {
        constexpr int WINDOW_SIZE = 65536;
        constexpr int LOOKAHEAD_SIZE = 255;
        constexpr int MIN_MATCH_LENGTH = 3;
        constexpr int HASH_TABLE_SIZE = 32768;
        constexpr int MAX_PROBES = 256;
    }

    // 3�o�C�g�̃n�b�V�����v�Z
    inline int CalculateHash(const std::vector<char>& data, int pos) {
        if ( pos + 2 >= data.size() ) return 0;
        unsigned int h = static_cast<unsigned char>( data[pos] );
        h = ( h << 5 ) ^ static_cast<unsigned char>( data[pos + 1] );
        h = ( h << 5 ) ^ static_cast<unsigned char>( data[pos + 2] );
        return h % HASH_TABLE_SIZE;
    }

    std::vector<Lz77Token> Lz77::Compress(const std::vector<char>& data) {
        std::vector<Lz77Token> tokens;
        if ( data.empty() ) return tokens;

        std::vector<int> head(HASH_TABLE_SIZE, -1);
        std::vector<int> prev(data.size(), -1);

        int cursor = 0;
        while ( cursor < data.size() ) {
            // 1. �܂����݂̈ʒu�ōŒ���v��T��
            int best_match_length = 0;
            int best_match_distance = 0;
            if ( cursor + MIN_MATCH_LENGTH <= data.size() ) {
                int hash = CalculateHash(data, cursor);
                int current_pos = head[hash];
                int probes = 0;
                while ( current_pos != -1 && cursor - current_pos < WINDOW_SIZE && probes < MAX_PROBES ) {
                    int current_match_length = 0;
                    while ( current_match_length < LOOKAHEAD_SIZE &&
                        cursor + current_match_length < data.size() &&
                        current_pos + current_match_length < data.size() &&
                        data[current_pos + current_match_length] == data[cursor + current_match_length] ) {
                        current_match_length++;
                    }
                    if ( current_match_length > best_match_length ) {
                        best_match_length = current_match_length;
                        best_match_distance = cursor - current_pos;
                    }
                    current_pos = prev[current_pos];
                    probes++;
                }
            }

            if ( best_match_length < MIN_MATCH_LENGTH ) best_match_length = 0;
            if ( cursor + best_match_length >= data.size() ) best_match_length = 0;

            // 2. �g�[�N���𐶐�
            if ( best_match_length > 0 ) {
                tokens.push_back({ (uint16_t)best_match_distance, (uint8_t)best_match_length, data[cursor + best_match_length] });
            }
            else {
                tokens.push_back({ 0, 0, data[cursor] });
            }

            // 3. ���������������n�b�V����}�����A�J�[�\����i�߂�
            int advance = ( best_match_length > 0 ) ? ( best_match_length + 1 ) : 1;
            for ( int i = 0; i < advance; ++i ) {
                if ( cursor + MIN_MATCH_LENGTH <= data.size() ) {
                    int hash = CalculateHash(data, cursor);
                    prev[cursor] = head[hash];
                    head[hash] = cursor;
                }
                cursor++;
                if ( cursor >= data.size() ) break;
            }
        }
        return tokens;
    }

    std::vector<char> Lz77::Decompress(const std::vector<Lz77Token>& tokens) {
        std::vector<char> decompressedData;

        for ( const auto& token : tokens ) {
            if ( token.length > 0 ) {
                // ��v�����������ꍇ�̃g�[�N�� (distance, length, nextChar)
                // ���ɏo�͂����f�[�^�̒�����A(distance)�����k�����ʒu����(length)�������R�s�[����
                int startIndex = decompressedData.size() - token.distance;
                for ( int i = 0; i < token.length; ++i ) {
                    decompressedData.push_back(decompressedData[startIndex + i]);
                }
                // �Ō��nextChar��ǉ�
                decompressedData.push_back(token.nextChar);
            }
            else {
                // ��v��������Ȃ������ꍇ�̃g�[�N�� (0, 0, nextChar)
                // nextChar�i���e�����j�����̂܂ܒǉ�
                decompressedData.push_back(token.nextChar);
            }
        }
        return decompressedData;
    }

    std::vector<char> Lz77::SerializeTokens(const std::vector<Lz77Token>& tokens) {
        std::vector<char> serializedData;
        // �e�g�[�N����4�o�C�g�ɂȂ�Ɖ��肵�ă��������m��
        serializedData.reserve(tokens.size() * sizeof(Lz77Token));

        for ( const auto& token : tokens ) {
            // distance (uint16_t -> 2 bytes)
            serializedData.push_back(static_cast<char>( ( token.distance >> 8 ) & 0xFF )); // ��ʃo�C�g
            serializedData.push_back(static_cast<char>( token.distance & 0xFF ));      // ���ʃo�C�g
            // length (uint8_t -> 1 byte)
            serializedData.push_back(static_cast<char>( token.length ));
            // nextChar (char -> 1 byte)
            serializedData.push_back(token.nextChar);
        }
        return serializedData;
    }

    std::vector<Lz77Token> Lz77::DeserializeTokens(const std::vector<char>& data) {
        std::vector<Lz77Token> tokens;
        if ( data.size() % sizeof(Lz77Token) != 0 ) {
            // �f�[�^�T�C�Y���s��
            return tokens;
        }
        tokens.reserve(data.size() / sizeof(Lz77Token));

        for ( size_t i = 0; i < data.size(); i += sizeof(Lz77Token) ) {
            uint16_t distance = ( static_cast<uint8_t>( data[i] ) << 8 ) | static_cast<uint8_t>( data[i + 1] );
            uint8_t length = static_cast<uint8_t>( data[i + 2] );
            char nextChar = data[i + 3];
            tokens.push_back({ distance, length, nextChar });
        }
        return tokens;
    }
}