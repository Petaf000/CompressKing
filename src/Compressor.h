#pragma once
#include <string>

class Compressor {
public:
    // ���k���������s����
    bool CompressFolder(const std::string& sourceFolder, const std::string& outputFile);
};