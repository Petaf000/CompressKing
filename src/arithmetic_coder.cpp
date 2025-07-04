#include "arithmetic_coder.h"
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric>
#include <cstdint>

namespace Cmp {
    // --- 定数 (変更なし) ---
    namespace {
        constexpr int PRECISION_BITS = 32;
        constexpr uint64_t MAX_VALUE = ( 1ULL << PRECISION_BITS );
        constexpr uint64_t ONE_QUARTER = MAX_VALUE / 4;
        constexpr uint64_t HALF = 2 * ONE_QUARTER;
        constexpr uint64_t THREE_QUARTERS = 3 * ONE_QUARTER;
    }

    // --- ヘルパークラス (変更なし) ---
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
        void Flush() {
            if ( bitCount > 0 ) {
                stream.push_back(buffer);
            }
        }
        std::vector<char> GetStream() {
            return stream;
        }
    private:
        std::vector<char> stream;
        char buffer = 0;
        int bitCount = 0;
    };

    class BitStreamReader {
    public:
        BitStreamReader(const std::vector<char>& stream) : stream_ref(stream), buffer(0), count(8) {}
        bool ReadBit() {
            if ( count == 8 ) {
                if ( byte_index >= stream_ref.size() ) {
                    return false;
                }
                buffer = static_cast<uint8_t>( stream_ref[byte_index++] );
                count = 0;
            }
            bool bit = ( buffer >> ( 7 - count ) ) & 1;
            count++;
            return bit;
        }
    private:
        const std::vector<char>& stream_ref;
        size_t byte_index = 0;
        uint8_t buffer;
        int count;
    };


    // --- 確率モデルの再設計 ---

    // オーダー0モデル: 従来のProbabilityModelに相当。単一の文脈における確率分布を管理する。
    class Order0Model {
    public:
        Order0Model() {
            // 最初は全シンボルが1回ずつ出現した状態として初期化（ゼロ頻度回避）
            freqs.assign(256, 1);
            UpdateCumulativeFreqs();
        }

        void UpdateForSymbol(unsigned char symbol) {
            freqs[symbol]++;
            UpdateCumulativeFreqs();
        }

        // シリアライズのために頻度データを直接設定する
        void SetFreqs(const std::vector<uint32_t>& new_freqs) {
            if ( new_freqs.size() == 256 ) {
                freqs = new_freqs;
                UpdateCumulativeFreqs();
            }
        }
        const std::vector<uint32_t>& GetFreqs() const { return freqs; }

        uint64_t GetTotalFreq() const { return totalFreq; }
        uint64_t GetLowFreq(unsigned char symbol) const { return cumulativeFreqs[symbol]; }
        uint64_t GetHighFreq(unsigned char symbol) const { return cumulativeFreqs[symbol + 1]; }

        unsigned char FindSymbol(uint64_t scaled_value) const {
            for ( int i = 0; i < 256; ++i ) {
                if ( scaled_value < cumulativeFreqs[i + 1] ) {
                    return static_cast<unsigned char>( i );
                }
            }
            return 255;
        }

    private:
        void UpdateCumulativeFreqs() {
            cumulativeFreqs.assign(257, 0);
            totalFreq = 0;
            for ( int i = 0; i < 256; ++i ) {
                cumulativeFreqs[i + 1] = cumulativeFreqs[i] + freqs[i];
            }
            totalFreq = cumulativeFreqs[256];
        }
        std::vector<uint32_t> freqs;
        std::vector<uint64_t> cumulativeFreqs;
        uint64_t totalFreq = 0;
    };

    // コンテキストモデル: 256個のオーダー0モデルを管理する。
    class ContextualModel {
    public:
        ContextualModel() : models(256) {} // 256個のコンテキスト用のモデルを生成

        void Build(const std::vector<char>& data) {
            if ( data.empty() ) return;

            // 最初の文字はコンテキストがないので、専用モデルを更新
            initial_model.UpdateForSymbol(static_cast<unsigned char>( data[0] ));

            // 2文字目以降
            for ( size_t i = 1; i < data.size(); ++i ) {
                unsigned char context = static_cast<unsigned char>( data[i - 1] );
                unsigned char symbol = static_cast<unsigned char>( data[i] );
                models[context].UpdateForSymbol(symbol);
            }
        }

        std::vector<char> Serialize() const {
            std::vector<char> modelData;
            // 最初にinitial_modelをシリアライズ
            for ( uint32_t freq : initial_model.GetFreqs() ) {
                modelData.push_back(( freq >> 24 ) & 0xFF);
                modelData.push_back(( freq >> 16 ) & 0xFF);
                modelData.push_back(( freq >> 8 ) & 0xFF);
                modelData.push_back(freq & 0xFF);
            }
            // 次に256個のコンテキストモデルをシリアライズ
            for ( const auto& model : models ) {
                for ( uint32_t freq : model.GetFreqs() ) {
                    modelData.push_back(( freq >> 24 ) & 0xFF);
                    modelData.push_back(( freq >> 16 ) & 0xFF);
                    modelData.push_back(( freq >> 8 ) & 0xFF);
                    modelData.push_back(freq & 0xFF);
                }
            }
            return modelData;
        }

        bool Deserialize(const std::vector<char>& modelData) {
            if ( modelData.size() != ( 256 + 1 ) * 256 * 4 ) return false;

            size_t read_ptr = 0;
            auto deserialize_one_model = [ & ] (Order0Model& model) {
                std::vector<uint32_t> freqs(256);
                for ( int i = 0; i < 256; ++i ) {
                    uint32_t freq = 0;
                    freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 24;
                    freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 16;
                    freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 8;
                    freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) );
                    freqs[i] = freq;
                }
                model.SetFreqs(freqs);
                };

            deserialize_one_model(initial_model);
            for ( auto& model : models ) {
                deserialize_one_model(model);
            }
            return true;
        }

        // コンテキストに応じたモデルを返す
        const Order0Model& GetModelForContext(unsigned char context) const {
            return models[context];
        }
        // 最初の文字用のモデルを返す
        const Order0Model& GetInitialModel() const {
            return initial_model;
        }

    private:
        std::vector<Order0Model> models;
        Order0Model initial_model;
    };

    // --- ArithmeticCoderクラスの実装 (コンテキストモデルを使うように変更) ---

    std::vector<char> ArithmeticCoder::Compress(const std::vector<char>& data) {
        if ( data.empty() ) return {};

        ContextualModel model;
        model.Build(data);
        std::vector<char> output = model.Serialize();

        uint32_t originalSize = data.size();
        output.push_back(( originalSize >> 24 ) & 0xFF);
        output.push_back(( originalSize >> 16 ) & 0xFF);
        output.push_back(( originalSize >> 8 ) & 0xFF);
        output.push_back(originalSize & 0xFF);

        BitStreamWriter writer;
        uint64_t low = 0;
        uint64_t high = MAX_VALUE - 1;
        int underflow_bits = 0;

        unsigned char context = 0; // コンテキスト変数を初期化

        for ( size_t i = 0; i < data.size(); ++i ) {
            unsigned char symbol = static_cast<unsigned char>( data[i] );

            // コンテキストに応じて適切なモデルを選択
            const Order0Model& current_model = ( i == 0 ) ? model.GetInitialModel() : model.GetModelForContext(context);

            const uint64_t range = high - low + 1;
            const uint64_t total = current_model.GetTotalFreq();

            uint64_t new_high = low + ( range * current_model.GetHighFreq(symbol) / total ) - 1;
            uint64_t new_low = low + ( range * current_model.GetLowFreq(symbol) / total );
            high = new_high;
            low = new_low;

            // 正規化処理 (変更なし)
            while ( true ) {
                if ( high < HALF ) {
                    writer.WriteBit(false);
                    for ( int j = 0; j < underflow_bits; ++j ) writer.WriteBit(true);
                    underflow_bits = 0;
                    low <<= 1;
                    high = ( high << 1 ) | 1;
                }
                else if ( low >= HALF ) {
                    writer.WriteBit(true);
                    for ( int j = 0; j < underflow_bits; ++j ) writer.WriteBit(false);
                    underflow_bits = 0;
                    low = ( low - HALF ) << 1;
                    high = ( ( high - HALF ) << 1 ) | 1;
                }
                else if ( low >= ONE_QUARTER && high < THREE_QUARTERS ) {
                    underflow_bits++;
                    low = ( low - ONE_QUARTER ) << 1;
                    high = ( ( high - ONE_QUARTER ) << 1 ) | 1;
                }
                else {
                    break;
                }
            }

            // 次のループのためにコンテキストを更新
            context = symbol;
        }

        // 終端処理 (変更なし)
        writer.WriteBit(low >= ONE_QUARTER);
        underflow_bits++;
        for ( int i = 0; i < underflow_bits; ++i ) {
            writer.WriteBit(low < HALF);
        }
        writer.Flush();

        std::vector<char> stream = writer.GetStream();
        output.insert(output.end(), stream.begin(), stream.end());
        return output;
    }

    std::vector<char> ArithmeticCoder::Decompress(const std::vector<char>& data) {
        const size_t model_size = ( 256 + 1 ) * 256 * 4;
        const size_t header_size = model_size + 4;
        if ( data.size() < header_size ) return {};

        ContextualModel model;
        const std::vector<char> modelData(data.begin(), data.begin() + model_size);
        if ( !model.Deserialize(modelData) ) {
            return {};
        }

        uint32_t originalSize = 0;
        size_t size_offset = model_size;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 24;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 16;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 8;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) );

        if ( originalSize == 0 ) return {};

        const std::vector<char> payload(data.begin() + header_size, data.end());
        BitStreamReader reader(payload);

        uint64_t value = 0;
        for ( int i = 0; i < PRECISION_BITS; ++i ) {
            value = ( value << 1 ) | reader.ReadBit();
        }

        std::vector<char> decompressedData;
        decompressedData.reserve(originalSize);
        uint64_t low = 0, high = MAX_VALUE - 1;

        unsigned char context = 0; // コンテキスト変数を初期化

        for ( uint32_t i = 0; i < originalSize; ++i ) {
            // コンテキストに応じて適切なモデルを選択
            const Order0Model& current_model = ( i == 0 ) ? model.GetInitialModel() : model.GetModelForContext(context);

            const uint64_t range = high - low + 1;
            const uint64_t total = current_model.GetTotalFreq();
            const uint64_t scaled_value = ( ( value - low + 1 ) * total - 1 ) / range;

            unsigned char symbol = current_model.FindSymbol(scaled_value);
            decompressedData.push_back(symbol);

            uint64_t new_high = low + ( range * current_model.GetHighFreq(symbol) / total ) - 1;
            uint64_t new_low = low + ( range * current_model.GetLowFreq(symbol) / total );
            high = new_high;
            low = new_low;

            // 正規化処理 (変更なし)
            while ( true ) {
                if ( high < HALF ) {
                    low <<= 1;
                    high = ( high << 1 ) | 1;
                    value = ( value << 1 ) | reader.ReadBit();
                }
                else if ( low >= HALF ) {
                    low = ( low - HALF ) << 1;
                    high = ( ( high - HALF ) << 1 ) | 1;
                    value = ( value - HALF ) << 1 | reader.ReadBit();
                }
                else if ( low >= ONE_QUARTER && high < THREE_QUARTERS ) {
                    low = ( low - ONE_QUARTER ) << 1;
                    high = ( ( high - ONE_QUARTER ) << 1 ) | 1;
                    value = ( value - ONE_QUARTER ) << 1 | reader.ReadBit();
                }
                else {
                    break;
                }
            }

            // 次のループのためにコンテキストを更新
            context = symbol;
        }
        return decompressedData;
    }
}