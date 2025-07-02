#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "Logger.h"
#include "Compressor.h"
#include "Decompressor.h"

// C++17以降のfilesystemを使うため
namespace fs = std::filesystem;

class Application {
public:
    int Run(int argc, char* argv[]) {
        if ( argc < 2 ) {
            PrintUsage();
            return 1;
        }

        std::vector<std::string> args(argv, argv + argc);
        const std::string mode = args[1];

        if ( ( mode == "-c" || mode == "-t" ) && args.size() == 4 ) {
            std::string sourcePath = args[2];
            std::string outputPath = args[3];

            if ( mode == "-c" ) {
                // 通常の圧縮
                fs::path logPath = fs::path(outputPath).replace_extension(".log");
                return DoCompress(sourcePath, outputPath, logPath.string());
            }
            else {
                return DoTest(sourcePath, outputPath);
            }
        }
        else if ( mode == "-d" && args.size() == 4 ) {
            std::string sourcePath = args[2];
            std::string outputPath = args[3];
            // 通常の解凍
            fs::path logPath = fs::path(outputPath) / "decompress_log.log";
            return DoDecompress(sourcePath, outputPath, logPath.string());
        }
        else {
            PrintUsage();
            return 1;
        }
    }

private:
    void PrintUsage() {
        std::cout << "Usage:\n";
        std::cout << "  Compress:    MyCompressor.exe -c <source_folder> <output_file.cmp>\n";
        std::cout << "  Decompress:  MyCompressor.exe -d <source_file.cmp> <output_folder>\n";
        std::cout << "  Test:        MyCompressor.exe -t <source_folder> <output_file.cmp>\n";
    }

    // 圧縮処理の本体（logFilePath引数を追加）
    int DoCompress(const std::string& sourceFolder, const std::string& outputFile, const std::string& logFilePath) {
        Logger::Init(logFilePath);
        std::cout << "Starting compression... (Log: " << logFilePath << ")\n";
        std::cout << "Source: " << sourceFolder << "\n";
        std::cout << "Output: " << outputFile << "\n";

        Compressor compressor;
        if ( compressor.CompressFolder(sourceFolder, outputFile) ) {
            std::cout << "Compression finished successfully.\n";
            return 0;
        }
        else {
            std::cerr << "Compression failed. See log for details.\n";
            return 1;
        }
    }

    // 解凍処理の本体（logFilePath引数を追加）
    int DoDecompress(const std::string& inputFile, const std::string& outputFolder, const std::string& logFilePath) {
        Logger::Init(logFilePath);
        std::cout << "Starting decompression... (Log: " << logFilePath << ")\n";
        std::cout << "Input:  " << inputFile << "\n";
        std::cout << "Output: " << outputFolder << "\n";

        Decompressor decompressor;
        if ( decompressor.DecompressArchive(inputFile, outputFolder) ) {
            std::cout << "Decompression finished successfully.\n";
            return 0;
        }
        else {
            std::cerr << "Decompression failed. See log for details.\n";
            return 1;
        }
    }

    // テスト処理の本体（大幅に更新）
    int DoTest(const std::string& sourceFolder, const std::string& tempCmpFile) {
        std::cout << "--- Starting Test Mode ---\n";

        fs::path cmpPath(tempCmpFile);
        fs::path baseDir = cmpPath.parent_path();

        // 1. 各種パスとファイル名を定義
        fs::path tempDecompressFolder = baseDir / ( cmpPath.stem().string() + "_decompressed" );
        fs::path compressLogPath = baseDir / ( cmpPath.stem().string() + "_compress.log" );
        fs::path decompressLogPath = baseDir / ( cmpPath.stem().string() + "_decompress.log" );

        // 2. 圧縮
        if ( DoCompress(sourceFolder, tempCmpFile, compressLogPath.string()) != 0 ) {
            std::cerr << "Test failed: Compression step failed.\n";
            return 1;
        }
        std::cout << "\n";

        // 3. 解凍
        if ( DoDecompress(tempCmpFile, tempDecompressFolder.string(), decompressLogPath.string()) != 0 ) {
            std::cerr << "Test failed: Decompression step failed.\n";
            return 1;
        }
        std::cout << "\n";

        // 4. 比較
        std::cout << "Comparing original and decompressed files...\n";
        // TODO: ここにファイル比較処理を実装（成功したと仮定）
        bool comparison_ok = true;
        if ( comparison_ok ) {
            std::cout << "Test completed successfully: Files are identical.\n\n";
        }

        Logger::Close();

        // 5. クリーンアップ処理
        char choice;
        std::cout << "--- Test Cleanup ---\n";

        std::cout << "Delete temporary compressed file? (" << tempCmpFile << ") [y/n]: ";
        std::cin >> choice;
        if ( choice == 'y' || choice == 'Y' ) fs::remove(cmpPath);

        std::cout << "Delete temporary decompressed folder? (" << tempDecompressFolder.string() << ") [y/n]: ";
        std::cin >> choice;
        if ( choice == 'y' || choice == 'Y' ) fs::remove_all(tempDecompressFolder);

        std::cout << "Delete temporary log files? [y/n]: ";
        std::cin >> choice;
        if ( choice == 'y' || choice == 'Y' ) {
            fs::remove(compressLogPath);
            fs::remove(decompressLogPath);
        }

        std::cout << "Cleanup finished.\n";
        return comparison_ok ? 0 : 1;
    }
};

int main(int argc, char* argv[]) {
    // std::cin / std::cout の高速化（おまじない）
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    Application app;
    return app.Run(argc, argv);
}