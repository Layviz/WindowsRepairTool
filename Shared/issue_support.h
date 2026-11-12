#pragma once

#include "core.h"
#include <chrono>

namespace wrt {

inline bool supported_platform(unsigned major, bool x64_process, bool x64_system, bool wine) {
    return major >= 10 && x64_process && x64_system && !wine;
}

inline std::wstring elapsed_seconds(std::chrono::milliseconds duration) {
    const auto count = std::max<std::int64_t>(0, duration.count());
    const auto fraction = (count % 1000) / 100;
    return std::to_wstring(count / 1000) + L"." + std::to_wstring(fraction);
}

// Tool output may split anywhere, including between a percentage's digits.
// Keep one bounded token, not a copy of the entire repair output.
class progress_reader {
public:
    std::optional<unsigned> feed(std::string_view bytes) {
        std::optional<unsigned> result;
        for (unsigned char ch : bytes) {
            if (ch == '%') {
                if (!overflow_) {
                    if (const auto value = parse_progress(token_ + '%')) result = value;
                }
                token_.clear();
                overflow_ = false;
            } else if (std::isalnum(ch) || ch == '.' || ch == ',') {
                if (token_.size() < 64) token_ += static_cast<char>(ch);
                else overflow_ = true;
            } else {
                token_.clear();
                overflow_ = false;
            }
        }
        return result;
    }
private:
    std::string token_;
    bool overflow_ = false;
};

class reboot_reader {
public:
    bool feed(std::string_view bytes) {
        // Match affirmative SFC messages only, not "no restart is required".
        for (unsigned char ch : bytes) {
            tail_ += ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch + 'a' - 'A') : static_cast<char>(ch);
            if (tail_.size() > 128) tail_.erase(0, tail_.size() - 128);
            for (const auto phrase : {"requires reboot", "requires a reboot", "after the next reboot",
                "must restart the computer", "restart your computer to complete", "einen neustart erfordert",
                "neustart erforderlich", "neu gestartet werden muss", "starten sie windows neu"}) {
                if (tail_.find(phrase) != std::string::npos) required_ = true;
            }
        }
        return required_;
    }
    bool required() const { return required_; }
private:
    std::string tail_;
    bool required_ = false;
};

inline std::wstring fit_status_line(std::wstring_view text, int columns) {
    if (columns <= 1) return {};
    const auto width = static_cast<std::size_t>(columns - 1); // Never wrap at the right edge.
    if (text.size() <= width) return std::wstring(text);
    if (width <= 3) return std::wstring(text.substr(0, width));
    return std::wstring(text.substr(0, width - 3)) + L"...";
}

inline unsigned overall_progress(std::size_t step, std::size_t total, unsigned percent) {
    if (!total) return 0;
    if (step >= total) return 100;
    return static_cast<unsigned>((static_cast<double>(step) + std::min(percent, 100u) / 100.0)
        * 100.0 / static_cast<double>(total));
}

inline std::string batch_shortcut(unsigned mode) {
    if (mode < 1 || mode > 3) return {};
    return "@echo off\r\n\"%~dp0..\\Binaries\\WRT.exe\" " + std::to_string(mode) +
        "\r\nexit /b %errorlevel%\r\n";
}

} // namespace wrt
