#include "huffman.h"
#include <map>
#include <queue>
#include <vector>
#include <functional>
#include <string>
#include <stdexcept>

namespace Cmp {
    // --- �w���p�[ ---
    struct NodeComparer {
        bool operator()(const std::unique_ptr<HuffmanNode>& a, const std::unique_ptr<HuffmanNode>& b) const {
            return a->freq > b->freq;
        }
    };

    // �r�b�g�P�ʂ̏������݂�⏕����N���X
    class BitStreamWriter {
    public:
        void WriteBit(bool bit) {
            buffer |= ( bit << ( 7 - bitCount ) );
            bitCount++;
            if ( bitCount == 8 ) {
                stream.push_back(buffer);
                buffer = 0;
                bitCount = 0;
            }
        }
        void WriteByte(char byte) {
            for ( int i = 7; i >= 0; --i ) {
                WriteBit(( byte >> i ) & 1);
            }
        }
        std::vector<char> GetStream() {
            if ( bitCount > 0 ) {
                stream.push_back(buffer);
            }
            return stream;
        }
    private:
        std::vector<char> stream;
        char buffer = 0;
        int bitCount = 0;
    };

    // �r�b�g�P�ʂ̓ǂݍ��݂�⏕����N���X
    class BitStreamReader {
    public:
        BitStreamReader(const std::vector<char>& stream) : stream(stream) {}

        bool ReadBit() {
            if ( byteIndex >= stream.size() ) {
                throw std::out_of_range("Reading past end of stream");
            }
            bool bit = ( stream[byteIndex] >> ( 7 - bitIndex ) ) & 1;
            bitIndex++;
            if ( bitIndex == 8 ) {
                bitIndex = 0;
                byteIndex++;
            }
            return bit;
        }

        char ReadByte() {
            char byte = 0;
            for ( int i = 7; i >= 0; --i ) {
                if ( ReadBit() ) {
                    byte |= ( 1 << i );
                }
            }
            return byte;
        }
    private:
        const std::vector<char>& stream;
        size_t byteIndex = 0;
        int bitIndex = 0;
    };

    void GenerateCodes(const HuffmanNode* node, const std::string& code, std::map<char, std::string>& codeTable) {
        if ( !node ) return;
        if ( !node->left && !node->right ) codeTable[node->data] = code.empty() ? "0" : code;
        GenerateCodes(node->left.get(), code + "0", codeTable);
        GenerateCodes(node->right.get(), code + "1", codeTable);
    }

    // �؂̍\�����V���A���C�Y����
    void SerializeTree(const HuffmanNode* node, BitStreamWriter& writer) {
        if ( !node ) return;
        if ( !node->left && !node->right ) {
            writer.WriteBit(true); // �t�m�[�h��'1'
            writer.WriteByte(node->data);
        }
        else {
            writer.WriteBit(false); // �����m�[�h��'0'
            SerializeTree(node->left.get(), writer);
            SerializeTree(node->right.get(), writer);
        }
    }

    // �V���A���C�Y���ꂽ�f�[�^����؂𕜌�����
    std::unique_ptr<HuffmanNode> DeserializeTree(BitStreamReader& reader) {
        if ( reader.ReadBit() ) { // '1'�Ȃ�t�m�[�h
            return std::make_unique<HuffmanNode>(HuffmanNode{ reader.ReadByte() });
        }
        else { // '0'�Ȃ�����m�[�h
            auto left = DeserializeTree(reader);
            auto right = DeserializeTree(reader);
            return std::make_unique<HuffmanNode>(HuffmanNode{ '\0', 0, std::move(left), std::move(right) });
        }
    }

    // --- Huffman�N���X�̎��� ---
    std::vector<char> Huffman::Compress(const std::vector<char>& data) {
        if ( data.empty() ) return {};

        // 1. �p�x�v�Z & 2. �L���[�\�z & 3. �؂̍\�z (�ύX�Ȃ�)
        std::map<char, int> freqMap;
        for ( char c : data ) freqMap[c]++;

        std::priority_queue<std::unique_ptr<HuffmanNode>, std::vector<std::unique_ptr<HuffmanNode>>, NodeComparer> pq;
        for ( auto const& [key, val] : freqMap ) {
            pq.push(std::make_unique<HuffmanNode>(HuffmanNode{ key, val }));
        }

        // �f�[�^��1��ނ����Ȃ��ꍇ�̑Ώ�
        if ( pq.size() == 1 ) {
            auto single_node = std::move(const_cast<std::unique_ptr<HuffmanNode>&>( pq.top() ));
            pq.pop();
            auto internalNode = std::make_unique<HuffmanNode>();
            internalNode->freq = single_node->freq;
            internalNode->left = std::move(single_node);
            pq.push(std::move(internalNode));
        }

        while ( pq.size() > 1 ) {
            auto left = std::move(const_cast<std::unique_ptr<HuffmanNode>&>( pq.top() )); pq.pop();
            auto right = std::move(const_cast<std::unique_ptr<HuffmanNode>&>( pq.top() )); pq.pop();
            auto internalNode = std::make_unique<HuffmanNode>(HuffmanNode{ '\0', left->freq + right->freq, std::move(left), std::move(right) });
            pq.push(std::move(internalNode));
        }

        std::unique_ptr<HuffmanNode> root = std::move(const_cast<std::unique_ptr<HuffmanNode>&>( pq.top() ));
        pq.pop();

        // 4. �����\�̍쐬 (�ύX�Ȃ�)
        std::map<char, std::string> codeTable;
        GenerateCodes(root.get(), "", codeTable);

        // 5. �؂̍\���ƕ��������ꂽ�f�[�^���o�C�g��ɂ܂Ƃ߂�
        BitStreamWriter writer;

        // (a) �؂̍\������������
        SerializeTree(root.get(), writer);

        // (b) ���f�[�^�̃T�C�Y(uint32_t)����������
        uint32_t originalSize = static_cast<uint32_t>( data.size() );
        writer.WriteByte(( originalSize >> 24 ) & 0xFF);
        writer.WriteByte(( originalSize >> 16 ) & 0xFF);
        writer.WriteByte(( originalSize >> 8 ) & 0xFF);
        writer.WriteByte(originalSize & 0xFF);

        // (c) �����������f�[�^����������
        for ( char c : data ) {
            for ( char bit : codeTable[c] ) {
                writer.WriteBit(bit == '1');
            }
        }

        return writer.GetStream();
    }

    std::vector<char> Huffman::Decompress(const std::vector<char>& data) {
        if ( data.empty() ) return {};

        try {
            BitStreamReader reader(data);

            // 1. �o�C�g�񂩂�؂̍\���𕜌�����
            auto root = DeserializeTree(reader);
            if ( !root ) return {};

            // 2. ���̃f�[�^�T�C�Y(uint32_t)��ǂݍ���
            uint32_t originalSize = 0;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 24;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 16;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 8;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) );

            // 3. ���������؂��g���A�f�[�^���f�R�[�h����
            std::vector<char> decompressedData;
            decompressedData.reserve(originalSize);

            HuffmanNode* currentNode = root.get();
            for ( uint32_t i = 0; i < originalSize; ++i ) {
                currentNode = root.get();
                // �t�m�[�h�ɓ��B����܂Ŗ؂����ǂ�
                while ( currentNode->left || currentNode->right ) {
                    if ( reader.ReadBit() ) { // '1'�Ȃ�E��
                        currentNode = currentNode->right.get();
                    }
                    else { // '0'�Ȃ獶��
                        currentNode = currentNode->left.get();
                    }
                }
                decompressedData.push_back(currentNode->data);
            }

            return decompressedData;

        }
        catch ( const std::out_of_range& e ) {
            // �f�[�^���s���Ŕ͈͊O�A�N�Z�X�����ꍇ
            return {};
        }
    }
}