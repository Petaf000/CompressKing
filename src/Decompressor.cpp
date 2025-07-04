#include "Decompressor.h"
#include "FileFormat.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "lz77.h"
#include "rle.h"
#include "delta.h"
#include "bwt.h"
#include "mtf.h"
#include "huffman.h"

#include "exe_filter.h"
#include "arithmetic_coder.h"

namespace fs = std::filesystem;

bool Decompressor::DecompressArchive(const std::string& inputFile, const std::string& outputFolder) {
    Logger::Info("Decompression process started for file: {}", inputFile);

    // 1. ���̓t�@�C�����J��
    std::ifstream inFile(inputFile, std::ios::binary);
    if ( !inFile.is_open() ) {
        Logger::Error("Failed to open input file: {}", inputFile);
        std::cerr << "Error: Failed to open input file " << inputFile << std::endl;
        return false;
    }

    // 2. �S�̃w�b�_��ǂݍ��݁A���؂���
    Cmp::GlobalHeader header;
    inFile.read(reinterpret_cast<char*>( &header ), sizeof(header));

    if ( std::string(header.magic, 4) != "CMPC" ) {
        Logger::Error("Invalid file format or not a CMPC file.");
        std::cerr << "Error: Invalid file format." << std::endl;
        return false;
    }
    Logger::Info("Global header read. Version: {}, File count: {}", header.version, header.fileCount);

    // 3. �o�͐�t�H���_���쐬
    fs::create_directories(outputFolder);

    // 4. �t�@�C�����������[�v���Ċe�t�@�C������
    for ( uint8_t i = 0; i < header.fileCount; ++i ) {
        // (a) �t�@�C���G���g���w�b�_��ǂݍ���
        Cmp::FileEntryHeader entryHeader;
        inFile.read(reinterpret_cast<char*>( &entryHeader ), sizeof(entryHeader));
        if ( inFile.gcount() != sizeof(entryHeader) ) {
            Logger::Error("Failed to read file entry header for file #{}", i + 1);
            return false; // �w�b�_���ǂ߂Ȃ���Βv���I
        }

        // (b) �t�@�C������ǂݍ���
        std::string relativePath(entryHeader.fileNameLength, '\0');
        inFile.read(relativePath.data(), entryHeader.fileNameLength);

        // (c) �t�@�C���f�[�^��ǂݍ���
        std::vector<char> compressedData(entryHeader.compressedSize);
        inFile.read(compressedData.data(), entryHeader.compressedSize);

        Logger::Info("Decompressing [{} / {}]: Path: '{}', Size: {}", i + 1, header.fileCount, relativePath, entryHeader.compressedSize);

        // (d) �A���S���Y���ɉ����ĉ𓀏���
        std::vector<char> decompressedData;
        bool success = true;

        // ����������ύX��
        auto algorithm = static_cast<Cmp::Algorithm>( entryHeader.algorithmId );
        if ( algorithm == Cmp::Algorithm::STORE ) {
            // STORE���[�h�Ȃ̂ŁA�ǂݍ��񂾃f�[�^�����̂܂܌��̃f�[�^
            decompressedData = compressedData;
        }
        else if ( algorithm == Cmp::Algorithm::LZ77_HUFFMAN ) {
            // ����������ύX��
            // 1. Huffman�𓀂�LZ77�̃o�C�g��ɖ߂�
            auto lz77_bytes = Cmp::ArithmeticCoder::Decompress(compressedData);

            // 2. �o�C�g����g�[�N�����X�g�Ƀf�V���A���C�Y
            auto lz77_tokens = Cmp::Lz77::DeserializeTokens(lz77_bytes);
            if ( lz77_tokens.empty() && !compressedData.empty() ) {
                Logger::Error("  -> LZ77 Deserialization failed.");
                success = false;
            }
            else {
                // 2. �g�[�N�����X�g���猳�̃f�[�^�𕜌�
                decompressedData = Cmp::Lz77::Decompress(lz77_tokens);
            }
        }
        else if ( algorithm == Cmp::Algorithm::RLE_HUFFMAN ) { // �� ���̃u���b�N��ǉ�
            auto rle_bytes = Cmp::ArithmeticCoder::Decompress(compressedData);
            decompressedData = Cmp::Rle::Decompress(rle_bytes);
        }
        else if ( algorithm == Cmp::Algorithm::DELTA_HUFFMAN ) {
            // 1. Huffman��
            auto lz77_bytes = Cmp::ArithmeticCoder::Decompress(compressedData);
            // 2. LZ77��
            auto lz77_tokens = Cmp::Lz77::DeserializeTokens(lz77_bytes);
            auto delta_bytes = Cmp::Lz77::Decompress(lz77_tokens);
            // 3. �Ō��Delta�t�ϊ�
            decompressedData = Cmp::Delta::Decompress(delta_bytes);
        }
        else if ( algorithm == Cmp::Algorithm::BWT_HUFFMAN ) {
            // 1. Huffman -> MTF�t�ϊ�
            auto mtf_bytes = Cmp::ArithmeticCoder::Decompress(compressedData);
            auto bwt_serialized = Cmp::Mtf::InverseTransform(mtf_bytes);

            // 2. index��BWT�f�[�^�𕪗�
            uint32_t index = 0;
            index |= static_cast<uint32_t>( static_cast<uint8_t>( bwt_serialized[0] ) ) << 24;
            index |= static_cast<uint32_t>( static_cast<uint8_t>( bwt_serialized[1] ) ) << 16;
            index |= static_cast<uint32_t>( static_cast<uint8_t>( bwt_serialized[2] ) ) << 8;
            index |= static_cast<uint32_t>( static_cast<uint8_t>( bwt_serialized[3] ) );

            std::vector<char> bwt_data(bwt_serialized.begin() + 4, bwt_serialized.end());

            // 3. BWT�t�ϊ�
            decompressedData = Cmp::Bwt::InverseTransform({ bwt_data, index });
        }
        else if ( algorithm == Cmp::Algorithm::EXE_FILTER_LZ77_HUFFMAN ) { // �� ���̃u���b�N��ǉ�
            auto lz77_bytes = Cmp::ArithmeticCoder::Decompress(compressedData);
            auto lz77_tokens = Cmp::Lz77::DeserializeTokens(lz77_bytes);
            auto filtered_data = Cmp::Lz77::Decompress(lz77_tokens);
            decompressedData = Cmp::ExeFilter::InverseTransform(filtered_data);
        }
        else {
            Logger::Error("  -> Unsupported algorithm ID: {}", entryHeader.algorithmId);
            success = false;
        }

        // (e) �t�@�C���ɏ����o��
        if ( success && decompressedData.size() != entryHeader.originalSize ) {
            Logger::Error("  -> Decompression size mismatch. Expected: {}, Actual: {}", entryHeader.originalSize, decompressedData.size());
            success = false;
        }

        if ( success ) {
            fs::path finalOutputPath = fs::path(outputFolder) / relativePath;

            if ( finalOutputPath.has_parent_path() ) {
                fs::create_directories(finalOutputPath.parent_path());
            }

            std::ofstream outFile(finalOutputPath, std::ios::binary);
            if ( !outFile.is_open() ) {
                Logger::Error("  -> Failed to create output file: {}", finalOutputPath.string());
                continue; // ���̃t�@�C���̏����𑱂���
            }
            outFile.write(decompressedData.data(), decompressedData.size());
            outFile.close();
            Logger::Info("  -> File successfully restored.");
        }
        else {
            // ���s�����ꍇ�͎��̃t�@�C���̏����𑱂���
            continue;
        }
    }

    Logger::Info("Decompression process successfully finished.");
    return true;
}