#include "huffman.h"
#include <map>
#include <queue>
#include <vector>
#include <functional>
#include <string>
#include <stdexcept>

namespace Cmp {
    // --- ヘルパー ---
    struct NodeComparer {
        bool operator()(const std::unique_ptr<HuffmanNode>& a, const std::unique_ptr<HuffmanNode>& b) const {
            return a->freq > b->freq;
        }
    };

    // ビット単位の書き込みを補助するクラス
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

    // ビット単位の読み込みを補助するクラス
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

    // 木の構造をシリアライズする
    void SerializeTree(const HuffmanNode* node, BitStreamWriter& writer) {
        if ( !node ) return;
        if ( !node->left && !node->right ) {
            writer.WriteBit(true); // 葉ノードは'1'
            writer.WriteByte(node->data);
        }
        else {
            writer.WriteBit(false); // 内部ノードは'0'
            SerializeTree(node->left.get(), writer);
            SerializeTree(node->right.get(), writer);
        }
    }

    // シリアライズされたデータから木を復元する
    std::unique_ptr<HuffmanNode> DeserializeTree(BitStreamReader& reader) {
        if ( reader.ReadBit() ) { // '1'なら葉ノード
            return std::make_unique<HuffmanNode>(HuffmanNode{ reader.ReadByte() });
        }
        else { // '0'なら内部ノード
            auto left = DeserializeTree(reader);
            auto right = DeserializeTree(reader);
            return std::make_unique<HuffmanNode>(HuffmanNode{ '\0', 0, std::move(left), std::move(right) });
        }
    }

    // --- Huffmanクラスの実装 ---
    std::vector<char> Huffman::Compress(const std::vector<char>& data) {
        if ( data.empty() ) return {};

        // 1. 頻度計算 & 2. キュー構築 & 3. 木の構築 (変更なし)
        std::map<char, int> freqMap;
        for ( char c : data ) freqMap[c]++;

        std::priority_queue<std::unique_ptr<HuffmanNode>, std::vector<std::unique_ptr<HuffmanNode>>, NodeComparer> pq;
        for ( auto const& [key, val] : freqMap ) {
            pq.push(std::make_unique<HuffmanNode>(HuffmanNode{ key, val }));
        }

        // データが1種類しかない場合の対処
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

        // 4. 符号表の作成 (変更なし)
        std::map<char, std::string> codeTable;
        GenerateCodes(root.get(), "", codeTable);

        // 5. 木の構造と符号化されたデータをバイト列にまとめる
        BitStreamWriter writer;

        // (a) 木の構造を書き込む
        SerializeTree(root.get(), writer);

        // (b) 元データのサイズ(uint32_t)を書き込む
        uint32_t originalSize = static_cast<uint32_t>( data.size() );
        writer.WriteByte(( originalSize >> 24 ) & 0xFF);
        writer.WriteByte(( originalSize >> 16 ) & 0xFF);
        writer.WriteByte(( originalSize >> 8 ) & 0xFF);
        writer.WriteByte(originalSize & 0xFF);

        // (c) 符号化したデータを書き込む
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

            // 1. バイト列から木の構造を復元する
            auto root = DeserializeTree(reader);
            if ( !root ) return {};

            // 2. 元のデータサイズ(uint32_t)を読み込む
            uint32_t originalSize = 0;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 24;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 16;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) ) << 8;
            originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( reader.ReadByte() ) );

            // 3. 復元した木を使い、データをデコードする
            std::vector<char> decompressedData;
            decompressedData.reserve(originalSize);

            HuffmanNode* currentNode = root.get();
            for ( uint32_t i = 0; i < originalSize; ++i ) {
                currentNode = root.get();
                // 葉ノードに到達するまで木をたどる
                while ( currentNode->left || currentNode->right ) {
                    if ( reader.ReadBit() ) { // '1'なら右へ
                        currentNode = currentNode->right.get();
                    }
                    else { // '0'なら左へ
                        currentNode = currentNode->left.get();
                    }
                }
                decompressedData.push_back(currentNode->data);
            }

            return decompressedData;

        }
        catch ( const std::out_of_range& e ) {
            // データが不正で範囲外アクセスした場合
            return {};
        }
    }
}