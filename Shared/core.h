#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
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

// One argument of a localized format string: a number or a wide string. The
// templated constructor stores every integer type without conversion traps
// (std::variant would reject "int" to unsigned as narrowing and find smaller
// unsigned types ambiguous between two numeric alternatives).
struct format_value {
    enum class kind { signed_number, unsigned_number, string };
    format_value() = delete;
    template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
    format_value(T number) : kind_(std::is_signed_v<T> ? kind::signed_number : kind::unsigned_number) {
        if (std::is_signed_v<T>) signed_number_ = static_cast<long long>(number);
        else unsigned_number_ = static_cast<unsigned long long>(number);
    }
    format_value(std::wstring_view text) : kind_(kind::string), string_(text) {}
    // std::wstring reaches std::wstring_view only through a user-defined
    // conversion operator, which cannot chain with this user-defined
    // constructor — so it needs its own overload.
    format_value(const std::wstring& text) : kind_(kind::string), string_(text) {}
    // String literals arrive as arrays; without this constructor the array
    // would need two user-defined conversions in a row, which is ill-formed.
    format_value(const wchar_t* text)
        : kind_(kind::string), string_(text ? std::wstring_view(text) : std::wstring_view()) {}

    kind kind_;
    long long signed_number_ = 0;
    unsigned long long unsigned_number_ = 0;
    std::wstring_view string_;
};

// Substitutes arguments into a localized format string. Only "%lu" (number),
// "%ls" (wide string) and "%%" (literal %) are supported; unknown placeholders
// are copied verbatim so a broken translation cannot crash.
inline std::wstring format(std::wstring_view text, std::initializer_list<format_value> arguments = {}) {
    std::wstring result;
    std::size_t next = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const wchar_t ch = text[i];
        if (ch != L'%' || i + 1 >= text.size()) { result += ch; continue; }
        if (text[i + 1] == L'%') { result += L'%'; ++i; continue; }
        if (i + 2 < text.size() && text[i + 1] == L'l' && (text[i + 2] == L'u' || text[i + 2] == L's')) {
            if (next < arguments.size()) {
                const auto& argument = arguments.begin()[next];
                bool substituted = false;
                if (text[i + 2] == L'u') {
                    if (argument.kind_ == format_value::kind::signed_number) {
                        result += std::to_wstring(argument.signed_number_);
                        substituted = true;
                    } else if (argument.kind_ == format_value::kind::unsigned_number) {
                        result += std::to_wstring(argument.unsigned_number_);
                        substituted = true;
                    }
                } else if (argument.kind_ == format_value::kind::string) {
                    result += argument.string_;
                    substituted = true;
                }
                if (substituted) {
                    ++next;
                    i += 2;
                    continue;
                }
            }
        }
        result += ch; // Missing argument or type mismatch: keep the placeholder verbatim.
    }
    return result;
}

namespace detail {
inline bool is_digit(char ch) { return ch >= '0' && ch <= '9'; }
}

// Returns the last percentage token in tool output ("62%" or "62.3%") as a
// whole percent 0..100, or nullopt when the text contains none. '.' and ','
// are accepted as decimal marks, so "62,3%" is 62 — the digit before '%' is
// never read as its own token. Tokens glued to a word ("x3%") are ignored.
inline std::optional<unsigned> parse_progress(std::string_view text) {
    std::optional<unsigned> value;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!detail::is_digit(text[i])) { ++i; continue; }
        const std::size_t start = i;
        unsigned number = 0;
        bool overflow = false;
        while (i < text.size() && detail::is_digit(text[i])) {
            const unsigned next = number * 10 + static_cast<unsigned>(text[i] - '0');
            if (next > 1000) overflow = true;
            else number = next;
            ++i;
        }
        std::size_t end = i;
        // Up to two decimal digits: every percentage printout fits ("62.3%",
        // "13.75%"), and a thousands-separated "1,234%" still leaves its "34%"
        // as a word-glued token, which the boundary rule rejects.
        if (i + 1 < text.size() && (text[i] == '.' || text[i] == ',') && detail::is_digit(text[i + 1])) {
            i += 2;
            if (i < text.size() && detail::is_digit(text[i])) ++i;
            end = i;
        }
        if (i < text.size() && text[i] == '%') {
            const char previous = start ? text[start - 1] : ' ';
            const bool boundary = !std::isalnum(static_cast<unsigned char>(previous)) &&
                previous != '.' && previous != ',';
            if (!overflow && number <= 100 && boundary) value = number;
            ++i;
        } else {
            i = end;
        }
    }
    return value;
}

// Redraws a one-line status in place. Carriage return plus padding with
// spaces so that a shorter line fully covers its predecessor.
inline void redraw_line(std::wostream& out, std::wstring_view text, std::size_t& width) {
    out << L'\r' << text;
    if (text.size() < width) out << std::wstring(width - text.size(), L' ');
    else width = text.size();
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
