#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "../Windows Reparatur/wrt_types.h"

namespace wrt {

inline bool parse_operation(std::string_view input, OPERATION& operation) {
    operation = {};
    if (input.empty()) return true;
    if (input.size() > 2) return false;
    switch (input[0]) {
    case '1': operation.mode = SIMPLE_REPAIR; break;
    case '2': operation.mode = DEFAULT_REPAIR; break;
    case '3': operation.mode = EXT_REPAIR; break;
    case 'h': case '?': operation.mode = HELP; break;
    case '+': operation.mode = VERBOSE_ON; break;
    case '-': operation.mode = VERBOSE_OFF; break;
    default: return input.size() == 1;
    }
    if (input.size() == 2) {
        if (operation.mode < SIMPLE_REPAIR || operation.mode > EXT_REPAIR) return false;
        if (input[1] == '+') operation.verbose = VERBOSE;
        else if (input[1] == '-') operation.verbose = SILENT;
        else return false;
    }
    return true;
}

inline bool effective_verbose(VEROBOSITY setting, bool global) {
    return setting == VERBOSE || (setting == DEFAULT && global);
}

inline std::size_t console_padding(int columns, std::size_t width) {
    return columns > 0 && static_cast<std::size_t>(columns) > width
        ? (static_cast<std::size_t>(columns) - width) / 2 : 0;
}

inline std::wstring language_name(std::wstring_view locale) {
    const auto end = locale.find(L'-');
    const auto language = locale.substr(0, end);
    if (language.size() < 2 || language.size() > 3) return {};
    std::wstring result;
    for (wchar_t ch : language) {
        if (ch >= L'A' && ch <= L'Z') ch = static_cast<wchar_t>(ch + L'a' - L'A');
        if (ch < L'a' || ch > L'z') return {};
        result += ch;
    }
    return result;
}

// Quote one argument using the Windows CommandLineToArgvW/CRT rules, not cmd.exe.
inline std::wstring quote_argument(std::wstring_view value) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') { ++slashes; continue; }
        if (ch == L'\"') result.append(slashes * 2 + 1, L'\\');
        else result.append(slashes, L'\\');
        slashes = 0;
        result += ch;
    }
    result.append(slashes * 2, L'\\');
    return result + L'\"';
}

struct version {
    unsigned major = 0, minor = 0, patch = 0;
    std::uint32_t packed() const { return (major << 24) | (minor << 16) | patch; }
};

inline std::optional<version> parse_version(std::string_view input) {
    if (!input.empty() && input.front() == 'v') input.remove_prefix(1);
    std::array<unsigned, 3> parts{};
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const auto end = input.find('.');
        const auto part = input.substr(0, end);
        if (part.empty()) return std::nullopt;
        const unsigned limit = i == 2 ? 65535 : 255;
        for (char ch : part) {
            if (ch < '0' || ch > '9') return std::nullopt;
            const unsigned digit = static_cast<unsigned>(ch - '0');
            if (parts[i] > (limit - digit) / 10) return std::nullopt;
            parts[i] = parts[i] * 10 + digit;
        }
        if (i == 2) {
            if (end != std::string_view::npos) return std::nullopt;
        } else {
            if (end == std::string_view::npos) return std::nullopt;
            input.remove_prefix(end + 1);
        }
    }
    return version{parts[0], parts[1], parts[2]};
}

enum class tool { defrag, sfc, defender, chkdsk, dism, cleanup };
enum class risk { none, cleanup, surface_scan, offline_repair };

struct repair_step {
    tool program;
    std::wstring arguments;
    risk confirmation = risk::none;
};

inline std::vector<repair_step> repair_steps(OPTION mode, const std::wstring& drive) {
    if (mode == SIMPLE_REPAIR) return {
        {tool::defrag, drive + L" /o /h"},
        {tool::sfc, L"/scannow"},
        {tool::defender, L"-Scan -ScanType 1"},
        {tool::chkdsk, drive + L" /scan /perf /i"},
        {tool::dism, L"/online /cleanup-image /CheckHealth"},
        {tool::dism, L"/online /cleanup-image /ScanHealth"},
        {tool::dism, L"/online /cleanup-image /RestoreHealth"},
        {tool::chkdsk, drive + L" /scan /perf"},
        {tool::sfc, L"/scannow"},
        {tool::cleanup, L"/sagerun:151", risk::cleanup},
        {tool::defrag, drive + L" /o /h"}
    };
    if (mode == DEFAULT_REPAIR) return {
        {tool::defrag, drive + L" /o /h"},
        {tool::sfc, L"/scannow"},
        {tool::defender, L"-Scan -ScanType 2"},
        {tool::chkdsk, drive + L" /scan /perf"},
        {tool::dism, L"/online /cleanup-image /CheckHealth"},
        {tool::dism, L"/online /cleanup-image /ScanHealth"},
        {tool::dism, L"/online /cleanup-image /RestoreHealth"},
        {tool::sfc, L"/scannow"},
        {tool::defrag, L"/c /o /h /m"},
        {tool::cleanup, L"/sagerun:152", risk::cleanup},
        // Online /scan cannot be combined with an offline surface scan.
        {tool::chkdsk, drive + L" /f /r /b", risk::surface_scan}
    };
    if (mode == EXT_REPAIR) return {
        {tool::cleanup, L"/sagerun:153", risk::cleanup},
        {tool::defrag, drive + L" /o /h"},
        {tool::chkdsk, drive + L" /scan /perf /i"},
        {tool::sfc, L"/scannow"},
        {tool::chkdsk, drive + L" /spotfix", risk::offline_repair}
    };
    return {};
}

struct process_result {
    bool started = false;
    bool completed = false;
    std::uint32_t exit_code = 0;
    std::uint32_t error = 0;
};

enum class step_status { success, reboot_required, failed };

inline step_status classify_result(tool program, const process_result& result) {
    if (!result.started || !result.completed || result.error) return step_status::failed;
    if (program == tool::dism && result.exit_code == 3010) return step_status::reboot_required;
    if (result.exit_code == 0 || (program == tool::chkdsk && result.exit_code == 1))
        return step_status::success;
    // CHKDSK 2 is ambiguous (cleanup or a skipped repair), so do not claim success.
    return step_status::failed;
}

// Output is bytes, never an implicitly terminated C string. SFC emits UTF-16LE
// when redirected; retain an odd byte between reads, including a split BOM.
class output_decoder {
public:
    explicit output_decoder(tool program) : program_(program) {}
    std::string decode(std::string_view bytes) {
        if (bytes.empty()) return {};
        if (program_ != tool::sfc) return std::string(bytes);
        pending_.append(bytes.data(), bytes.size());
        std::string output;
        std::size_t i = 0;
        for (; i + 1 < pending_.size(); i += 2) {
            unsigned ch = static_cast<unsigned char>(pending_[i]) |
                (static_cast<unsigned char>(pending_[i + 1]) << 8);
            if (ch == 0xfeff) continue;
            if (ch >= 0xd800 && ch <= 0xdbff) {
                if (high_surrogate_) output += "\xef\xbf\xbd";
                high_surrogate_ = ch;
                continue;
            }
            if (ch >= 0xdc00 && ch <= 0xdfff && high_surrogate_) {
                ch = 0x10000 + ((high_surrogate_ - 0xd800) << 10) + ch - 0xdc00;
            } else {
                if (high_surrogate_) output += "\xef\xbf\xbd";
                if (ch >= 0xdc00 && ch <= 0xdfff) ch = 0xfffd;
            }
            high_surrogate_ = 0;
            if (ch < 0x80) output += static_cast<char>(ch);
            else if (ch < 0x800) {
                output += static_cast<char>(0xc0 | (ch >> 6));
                output += static_cast<char>(0x80 | (ch & 0x3f));
            } else if (ch < 0x10000) {
                output += static_cast<char>(0xe0 | (ch >> 12));
                output += static_cast<char>(0x80 | ((ch >> 6) & 0x3f));
                output += static_cast<char>(0x80 | (ch & 0x3f));
            } else {
                output += static_cast<char>(0xf0 | (ch >> 18));
                output += static_cast<char>(0x80 | ((ch >> 12) & 0x3f));
                output += static_cast<char>(0x80 | ((ch >> 6) & 0x3f));
                output += static_cast<char>(0x80 | (ch & 0x3f));
            }
        }
        pending_.erase(0, i);
        return output;
    }
    std::string finish() {
        std::string output;
        if (high_surrogate_) output += "\xef\xbf\xbd";
        if (!pending_.empty()) output += "\xef\xbf\xbd";
        high_surrogate_ = 0;
        pending_.clear();
        return output;
    }
private:
    tool program_;
    std::string pending_;
    unsigned high_surrogate_ = 0;
};

} // namespace wrt
