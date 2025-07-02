#include "Compressor.h"
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
//#include "huffman.h"

#include "exe_filter.h"
#include "arithmetic_coder.h"

namespace fs = std::filesystem;

bool Compressor::CompressFolder(const std::string& sourceFolder, const std::string& outputFile) {
    Logger::Info("Compression process started for folder: {}", sourceFolder);

    // 1. ���k�Ώۂ̃t�@�C�����X�g���쐬
    std::vector<fs::path> filesToCompress;
    try {
        for ( const auto& entry : fs::recursive_directory_iterator(sourceFolder) ) {
            if ( entry.is_regular_file() ) {
                filesToCompress.push_back(entry.path());
            }
        }
    }
    catch ( const fs::filesystem_error& e ) {
        Logger::Error("Failed to access source folder: {}", e.what());
        std::cerr << "Error: Failed to access source folder " << sourceFolder << std::endl;
        return false;
    }

    if ( filesToCompress.empty() ) {
        Logger::Info("No files found to compress.");
        std::cout << "Warning: No files found in the source folder." << std::endl;
        return true;
    }

    if ( filesToCompress.size() > 255 ) {
        Logger::Error("Too many files. A maximum of 255 files is supported.", filesToCompress.size());
        std::cerr << "Error: The number of files (" << filesToCompress.size() << ") exceeds the limit of 255." << std::endl;
        return false;
    }
    Logger::Info("Found {} files to compress.", filesToCompress.size());

    // 2. �o�̓t�@�C�����J��
    std::ofstream outFile(outputFile, std::ios::binary);
    if ( !outFile.is_open() ) {
        Logger::Error("Failed to open output file: {}", outputFile);
        std::cerr << "Error: Failed to open output file " << outputFile << std::endl;
        return false;
    }

    // 3. �S�̃w�b�_���쐬���ď�������
    Cmp::GlobalHeader header;
    header.magic[0] = 'C';
    header.magic[1] = 'M';
    header.magic[2] = 'P';
    header.magic[3] = 'C';
    header.version = 1;
    header.fileCount = static_cast<uint8_t>( filesToCompress.size() );

    outFile.write(reinterpret_cast<const char*>( &header ), sizeof(header));
    Logger::Info("Global header written. Version: {}, File count: {}", header.version, header.fileCount);

    // 4. �e�t�@�C�������[�v���ď�������
    for ( const auto& filePath : filesToCompress ) {
        Logger::Info("Processing file: {}", filePath.string());

        std::ifstream inFile(filePath, std::ios::binary);
        if ( !inFile.is_open() ) {
            Logger::Error("Failed to open source file: {}", filePath.string());
            continue;
        }

        std::vector<char> fileData(
            ( std::istreambuf_iterator<char>(inFile) ),
            std::istreambuf_iterator<char>()
        );
        inFile.close();

        // ������ �������炪�A���S���Y���I�����W�b�N�i�ŏI�Łj ������
        Cmp::Algorithm selectedAlgo;
        std::vector<char> compressedData;

        // ����̃t�@�C���^�C�v�ɑ΂��ẮA�œK�ȃA���S���Y�������ߑł�
        if ( filePath.extension() == ".txt" ) {
            Logger::Info("  -> Selecting BWT for text file...");
            selectedAlgo = Cmp::Algorithm::BWT_HUFFMAN;
            auto bwt_result = Cmp::Bwt::Transform(fileData);
            std::vector<char> bwt_serialized;
            uint32_t index = static_cast<uint32_t>( bwt_result.second );
            bwt_serialized.push_back(( index >> 24 ) & 0xFF);
            bwt_serialized.push_back(( index >> 16 ) & 0xFF);
            bwt_serialized.push_back(( index >> 8 ) & 0xFF);
            bwt_serialized.push_back(index & 0xFF);
            bwt_serialized.insert(bwt_serialized.end(), bwt_result.first.begin(), bwt_result.first.end());
            auto mtf_bytes = Cmp::Mtf::Transform(bwt_serialized);
            compressedData = Cmp::ArithmeticCoder::Compress(mtf_bytes);
        }
        else if ( filePath.filename() == "explosion.wav" ) {
            Logger::Info("  -> Selecting Delta+LZ77 for wave file...");
            selectedAlgo = Cmp::Algorithm::DELTA_HUFFMAN;
            auto delta_bytes = Cmp::Delta::Compress(fileData);
            auto lz77_tokens = Cmp::Lz77::Compress(delta_bytes);
            auto lz77_bytes = Cmp::Lz77::SerializeTokens(lz77_tokens);
            compressedData = Cmp::ArithmeticCoder::Compress(lz77_bytes);
        }
        else if ( filePath.extension() == ".exe" ) { // �� .exe �p�̕���𖾎��I�ɍ쐬
            selectedAlgo = Cmp::Algorithm::EXE_FILTER_LZ77_HUFFMAN;
            auto filtered_data = Cmp::ExeFilter::Transform(fileData);
            auto lz77_tokens = Cmp::Lz77::Compress(filtered_data);
            auto lz77_bytes = Cmp::Lz77::SerializeTokens(lz77_tokens);
            compressedData = Cmp::ArithmeticCoder::Compress(lz77_bytes);
        }
        else {
            // ��L�ȊO�i.bmp, .exe�Ȃǁj�́ARLE��LZ77�������ėǂ�����I��
            Logger::Info("  -> Performing trial compression (RLE vs LZ77)...");

            // ���s1: RLE+Huffman
            auto rle_bytes = Cmp::Rle::Compress(fileData);
            auto rle_result = Cmp::ArithmeticCoder::Compress(rle_bytes);

            // ���s2: LZ77+Huffman
            auto lz77_tokens = Cmp::Lz77::Compress(fileData);
            auto lz77_bytes = Cmp::Lz77::SerializeTokens(lz77_tokens);
            auto lz77_result = Cmp::ArithmeticCoder::Compress(lz77_bytes);

            // ���ʂ��r���đI��
            if ( rle_result.size() < lz77_result.size() ) {
                selectedAlgo = Cmp::Algorithm::RLE_HUFFMAN;
                compressedData = rle_result;
                Logger::Info("  -> Trial result: RLE selected (RLE: {}, LZ77: {}).", rle_result.size(), lz77_result.size());
            }
            else {
                selectedAlgo = Cmp::Algorithm::LZ77_HUFFMAN;
                compressedData = lz77_result;
                Logger::Info("  -> Trial result: LZ77 selected (RLE: {}, LZ77: {}).", rle_result.size(), lz77_result.size());
            }
        }

        // --- �w�b�_�쐬�Ə������ݏ����͕ύX�Ȃ� ---
        Cmp::FileEntryHeader entryHeader;
        entryHeader.algorithmId = static_cast<uint8_t>( selectedAlgo );
        entryHeader.originalSize = static_cast<uint32_t>( fileData.size() );
        entryHeader.compressedSize = static_cast<uint32_t>( compressedData.size() );

        std::string relativePathStr = fs::relative(filePath, sourceFolder).string();
        if ( relativePathStr.length() > 255 ) {
            Logger::Error("File path is too long (max 255): {}", relativePathStr);
            continue;
        }
        entryHeader.fileNameLength = static_cast<uint8_t>( relativePathStr.length() );

        outFile.write(reinterpret_cast<const char*>( &entryHeader ), sizeof(entryHeader));
        outFile.write(relativePathStr.c_str(), relativePathStr.length());
        outFile.write(compressedData.data(), compressedData.size());

        double ratio = ( compressedData.empty() ) ? 0 : (double)entryHeader.originalSize / entryHeader.compressedSize;
        Logger::Info("  -> Compressed. Ratio: {:.2f}:1, Size: {} -> {}, Path: '{}'",
            ratio, entryHeader.originalSize, entryHeader.compressedSize, relativePathStr);
    }

    Logger::Info("Compression process successfully finished.");
    outFile.close();
    return true;
}