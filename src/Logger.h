#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <format>

class Logger {
public:
    // ログファイルを初期化
    static bool Init(const std::string& logFilePath) {
        logFile.open(logFilePath, std::ios::out | std::ios::trunc);
        if ( !logFile.is_open() ) {
            return false;
        }
        return true;
    }

    // 通常のログを記録
    template<typename... Args>
    static void Info(const std::string_view format_str, Args&&... args) {
        if ( logFile.is_open() ) {
            auto now = std::chrono::system_clock::now();
            logFile << std::format("{:%Y-%m-%d %H:%M:%S}", now) << " [INFO] " << std::vformat(format_str, std::make_format_args(args...)) << std::endl;
        }
    }

    // エラーログを記録
    template<typename... Args>
    static void Error(const std::string_view format_str, Args&&... args) {
        if ( logFile.is_open() ) {
            auto now = std::chrono::system_clock::now();
            logFile << std::format("{:%Y-%m-%d %H:%M:%S}", now) << " [ERROR] " << std::vformat(format_str, std::make_format_args(args...)) << std::endl;
        }
    }

    // ログファイルを閉じる
    static void Close() {
        if ( logFile.is_open() ) {
            logFile.close();
        }
    }

private:
    inline static std::ofstream logFile;
};