#pragma once
#include <string>

class Decompressor {
public:
    // 解凍処理を実行する
    bool DecompressArchive(const std::string& inputFile, const std::string& outputFolder);
};
