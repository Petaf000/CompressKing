#pragma once
#include <string>

class Compressor {
public:
    // 圧縮処理を実行する
    bool CompressFolder(const std::string& sourceFolder, const std::string& outputFile);
};