#pragma once
#include <string>

class Decompressor {
public:
    // �𓀏��������s����
    bool DecompressArchive(const std::string& inputFile, const std::string& outputFolder);
};
