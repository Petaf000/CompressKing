#include "exe_filter.h"

namespace Cmp {
    // x86の相対CALL命令のオペコード
    constexpr unsigned char X86_OPCODE_CALL = 0xE8;

    std::vector<char> ExeFilter::Transform(const std::vector<char>& data) {
        std::vector<char> result = data;
        for ( size_t i = 0; i + 4 < result.size(); ++i ) {
            if ( static_cast<unsigned char>( result[i] ) == X86_OPCODE_CALL ) {
                // CALL命令に続く4バイトの相対アドレスを取得
                uint32_t addr = *reinterpret_cast<uint32_t*>( &result[i + 1] );
                // 行き先アドレスを計算
                uint32_t dest = ( i + 5 ) + addr;
                // 正規化（ここでは単純に元の値と変換後の値をXORするなどの簡単な変換）
                *reinterpret_cast<uint32_t*>( &result[i + 1] ) = dest;
            }
        }
        return result;
    }

    std::vector<char> ExeFilter::InverseTransform(const std::vector<char>& data) {
        std::vector<char> result = data;
        for ( size_t i = 0; i + 4 < result.size(); ++i ) {
            if ( static_cast<unsigned char>( result[i] ) == X86_OPCODE_CALL ) {
                // Transformと全く同じロジックを適用すると元に戻る
                uint32_t dest = *reinterpret_cast<uint32_t*>( &result[i + 1] );
                uint32_t addr = dest - ( i + 5 );
                *reinterpret_cast<uint32_t*>( &result[i + 1] ) = addr;
            }
        }
        return result;
    }
}