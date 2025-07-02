#include "exe_filter.h"

namespace Cmp {
    // x86�̑���CALL���߂̃I�y�R�[�h
    constexpr unsigned char X86_OPCODE_CALL = 0xE8;

    std::vector<char> ExeFilter::Transform(const std::vector<char>& data) {
        std::vector<char> result = data;
        for ( size_t i = 0; i + 4 < result.size(); ++i ) {
            if ( static_cast<unsigned char>( result[i] ) == X86_OPCODE_CALL ) {
                // CALL���߂ɑ���4�o�C�g�̑��΃A�h���X���擾
                uint32_t addr = *reinterpret_cast<uint32_t*>( &result[i + 1] );
                // �s����A�h���X���v�Z
                uint32_t dest = ( i + 5 ) + addr;
                // ���K���i�����ł͒P���Ɍ��̒l�ƕϊ���̒l��XOR����Ȃǂ̊ȒP�ȕϊ��j
                *reinterpret_cast<uint32_t*>( &result[i + 1] ) = dest;
            }
        }
        return result;
    }

    std::vector<char> ExeFilter::InverseTransform(const std::vector<char>& data) {
        std::vector<char> result = data;
        for ( size_t i = 0; i + 4 < result.size(); ++i ) {
            if ( static_cast<unsigned char>( result[i] ) == X86_OPCODE_CALL ) {
                // Transform�ƑS���������W�b�N��K�p����ƌ��ɖ߂�
                uint32_t dest = *reinterpret_cast<uint32_t*>( &result[i + 1] );
                uint32_t addr = dest - ( i + 5 );
                *reinterpret_cast<uint32_t*>( &result[i + 1] ) = addr;
            }
        }
        return result;
    }
}