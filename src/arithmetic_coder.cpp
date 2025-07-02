#include "arithmetic_coder.h"
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric> // std::accumulate

namespace Cmp {
    // --- 定数 ---
    namespace {
        constexpr int PRECISION_BITS = 32;
        constexpr uint64_t MAX_VALUE = ( 1ULL << PRECISION_BITS );
        constexpr uint64_t ONE_QUARTER = MAX_VALUE / 4;
        constexpr uint64_t HALF = 2 * ONE_QUARTER;
        constexpr uint64_t THREE_QUARTERS = 3 * ONE_QUARTER;
    }

    // --- ヘルパークラス ---
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
                buffer = stream_ref[byte_index++];
                count = 0;
            }
            bool bit = ( buffer >> ( 7 - count ) ) & 1;
            count++;
            return bit;
        }
    private:
        const std::vector<char>& stream_ref; // 参照として保持
        size_t byte_index = 0;
        char buffer;
        int count;
    };

    class ProbabilityModel {
    public:
        void Build(const std::vector<char>& data) {
            freqs.assign(256, 0);
            for ( unsigned char c : data ) {
                freqs[c]++;
            }
            // ゼロ頻度回避のため、全シンボルに1を加算（スムージング）
            for ( uint32_t& f : freqs ) {
                f++;
            }
            UpdateCumulativeFreqs();
        }

        std::vector<char> Serialize() const {
            std::vector<char> modelData;
            modelData.reserve(256 * 4);
            for ( uint32_t freq : freqs ) {
                modelData.push_back(( freq >> 24 ) & 0xFF);
                modelData.push_back(( freq >> 16 ) & 0xFF);
                modelData.push_back(( freq >> 8 ) & 0xFF);
                modelData.push_back(freq & 0xFF);
            }
            return modelData;
        }

        bool Deserialize(const std::vector<char>& modelData) {
            if ( modelData.size() != 256 * 4 ) return false;
            freqs.assign(256, 0);
            size_t read_ptr = 0;
            for ( int i = 0; i < 256; ++i ) {
                uint32_t freq = 0;
                freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 24;
                freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 16;
                freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) ) << 8;
                freq |= static_cast<uint32_t>( static_cast<uint8_t>( modelData[read_ptr++] ) );
                freqs[i] = freq;
            }
            UpdateCumulativeFreqs();
            return true;
        }

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

    // --- ArithmeticCoderクラスの実装 ---
    std::vector<char> ArithmeticCoder::Compress(const std::vector<char>& data) {
        if ( data.empty() ) return {};

        ProbabilityModel model;
        model.Build(data);
        std::vector<char> output = model.Serialize();

        uint32_t originalSize = data.size();
        output.push_back(( originalSize >> 24 ) & 0xFF);
        output.push_back(( originalSize >> 16 ) & 0xFF);
        output.push_back(( originalSize >> 8 ) & 0xFF);
        output.push_back(originalSize & 0xFF);

        BitStreamWriter writer;
        uint64_t low = 0;
        uint64_t high = MAX_VALUE;
        int underflow_bits = 0;

        for ( unsigned char symbol : data ) {
            const uint64_t range = high - low;
            const uint64_t total = model.GetTotalFreq();
            high = low + ( range * model.GetHighFreq(symbol) / total );
            low = low + ( range * model.GetLowFreq(symbol) / total );

            while ( true ) {
                if ( high <= HALF ) {
                    writer.WriteBit(false);
                    for ( int i = 0; i < underflow_bits; ++i ) writer.WriteBit(true);
                    underflow_bits = 0;
                    low <<= 1;
                    high <<= 1;
                }
                else if ( low >= HALF ) {
                    writer.WriteBit(true);
                    for ( int i = 0; i < underflow_bits; ++i ) writer.WriteBit(false);
                    underflow_bits = 0;
                    low = ( low - HALF ) << 1;
                    high = ( high - HALF ) << 1;
                }
                else if ( low >= ONE_QUARTER && high <= THREE_QUARTERS ) {
                    underflow_bits++;
                    low = ( low - ONE_QUARTER ) << 1;
                    high = ( high - ONE_QUARTER ) << 1;
                }
                else {
                    break;
                }
            }
        }

        writer.WriteBit(low >= ONE_QUARTER);
        writer.Flush();

        std::vector<char> stream = writer.GetStream();
        output.insert(output.end(), stream.begin(), stream.end());
        return output;
    }

    std::vector<char> ArithmeticCoder::Decompress(const std::vector<char>& data) {
        if ( data.size() < 1028 ) return {};

        ProbabilityModel model;
        const std::vector<char> modelData(data.begin(), data.begin() + 1024);
        if ( !model.Deserialize(modelData) ) {
            return {};
        }

        uint32_t originalSize = 0;
        size_t size_offset = 1024;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 24;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 16;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) ) << 8;
        originalSize |= static_cast<uint32_t>( static_cast<uint8_t>( data[size_offset++] ) );

        if ( originalSize == 0 ) return {};

        // ★★★★★ 最重要修正点 ★★★★★
        // 一時オブジェクトではなく、永続的な変数としてペイロードを保持する
        const std::vector<char> payload(data.begin() + 1028, data.end());
        BitStreamReader reader(payload); // これでreaderは有効なデータを参照する

        uint64_t value = 0;
        for ( int i = 0; i < PRECISION_BITS; ++i ) {
            value = ( value << 1 ) | reader.ReadBit();
        }

        std::vector<char> decompressedData;
        decompressedData.reserve(originalSize);
        uint64_t low = 0, high = MAX_VALUE;
        const uint64_t total = model.GetTotalFreq();

        for ( uint32_t i = 0; i < originalSize; ++i ) {
            const uint64_t range = high - low;
            // valueがlowを下回らないように保護
            const uint64_t current_code = ( value >= low ) ? ( value - low ) : 0;
            const uint64_t scaled_value = ( current_code * total ) / range;

            unsigned char symbol = model.FindSymbol(scaled_value);
            decompressedData.push_back(symbol);

            high = low + ( range * model.GetHighFreq(symbol) / total );
            low = low + ( range * model.GetLowFreq(symbol) / total );

            while ( true ) {
                if ( high <= HALF ) {
                    low <<= 1;
                    high <<= 1;
                    value = ( value << 1 ) | reader.ReadBit();
                }
                else if ( low >= HALF ) {
                    low = ( low - HALF ) << 1;
                    high = ( high - HALF ) << 1;
                    value = ( value - HALF ) << 1 | reader.ReadBit();
                }
                else if ( low >= ONE_QUARTER && high <= THREE_QUARTERS ) {
                    low = ( low - ONE_QUARTER ) << 1;
                    high = ( high - ONE_QUARTER ) << 1;
                    value = ( value - ONE_QUARTER ) << 1 | reader.ReadBit();
                }
                else {
                    break;
                }
            }
        }
        return decompressedData;
    }
}