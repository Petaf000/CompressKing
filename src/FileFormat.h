#pragma once
#include <cstdint>

// コンパイラによる余計なパディングを無効にし、ファイル構造とメモリ上の構造を一致させる
#pragma pack(push, 1)

namespace Cmp {
    // .cmpファイルの全体ヘッダ
    struct GlobalHeader {
        char magic[4];      // マジックナンバー "CMPC"
        uint8_t version;    // フォーマットバージョン
        uint8_t fileCount;  // ファイル数
    };

    // 各ファイルエントリのヘッダ
    struct FileEntryHeader {
        uint8_t algorithmId;        // アルゴリズムID
        uint8_t fileNameLength;     // ファイル名の長さ
        uint32_t originalSize;      // 元のファイルサイズ
        uint32_t compressedSize;    // 圧縮後のデータサイズ
    };

    // アルゴリズムIDの定義
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