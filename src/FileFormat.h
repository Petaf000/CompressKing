#pragma once
#include <cstdint>

// �R���p�C���ɂ��]�v�ȃp�f�B���O�𖳌��ɂ��A�t�@�C���\���ƃ�������̍\������v������
#pragma pack(push, 1)

namespace Cmp {
    // .cmp�t�@�C���̑S�̃w�b�_
    struct GlobalHeader {
        char magic[4];      // �}�W�b�N�i���o�[ "CMPC"
        uint8_t version;    // �t�H�[�}�b�g�o�[�W����
        uint8_t fileCount;  // �t�@�C����
    };

    // �e�t�@�C���G���g���̃w�b�_
    struct FileEntryHeader {
        uint8_t algorithmId;        // �A���S���Y��ID
        uint8_t fileNameLength;     // �t�@�C�����̒���
        uint32_t originalSize;      // ���̃t�@�C���T�C�Y
        uint32_t compressedSize;    // ���k��̃f�[�^�T�C�Y
    };

    // �A���S���Y��ID�̒�`
    enum class Algorithm : uint8_t {
        STORE = 0,
        LZ77_HUFFMAN = 1,
        RLE_HUFFMAN = 2,
        DELTA_HUFFMAN = 3,
        BWT_HUFFMAN = 4,
        EXE_FILTER_LZ77_HUFFMAN = 5,
    };
}

#pragma pack(pop)