#pragma once

#include "core.h"
#include "../WRTLauncher/json.hpp"

namespace wrt {

struct release_asset {
    std::string name;
    std::string request_path;
    std::uint64_t size = 0;
};

inline std::string manual_installer_url(const std::optional<release_asset>& asset) {
    return asset ? "https://github.com" + asset->request_path :
        "https://github.com/Layviz/WindowsRepairTool/releases/latest";
}

inline bool safe_msi_name(std::string_view name) {
    if (name.size() <= 4 || name.size() > 128 || name.substr(name.size() - 4) != ".msi")
        return false;
    for (char ch : name) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.')) return false;
    }
    return name.front() != '.' && name.find("..") == std::string_view::npos;
}

inline std::optional<release_asset> installer_asset(const nlohmann::json& release) {
    if (!release.is_object() || !release.contains("tag_name") ||
        !release["tag_name"].is_string() || !release.contains("assets") ||
        !release["assets"].is_array()) return std::nullopt;
    const auto tag = release["tag_name"].get<std::string>();
    if (!parse_version(tag)) return std::nullopt;
    const std::string prefix = "https://github.com/Layviz/WindowsRepairTool/releases/download/" + tag + "/";
    std::optional<release_asset> selected;
    for (const auto& asset : release["assets"]) {
        if (!asset.is_object() || !asset.contains("name") || !asset["name"].is_string() ||
            !asset.contains("browser_download_url") || !asset["browser_download_url"].is_string() ||
            !asset.contains("size") || !asset["size"].is_number_unsigned()) continue;
        const auto name = asset["name"].get<std::string>();
        const auto url = asset["browser_download_url"].get<std::string>();
        const auto size = asset["size"].get<std::uint64_t>();
        if (!safe_msi_name(name) || url != prefix + name || size == 0 || size > 512 * 1024 * 1024)
            continue;
        if (selected) return std::nullopt; // Never guess between different installers.
        selected = release_asset{name, url.substr(std::string("https://github.com").size()), size};
    }
    return selected;
}

inline bool successful_install_exit(std::uint32_t code) {
    return code == 0 || code == 3010 || code == 1641;
}

inline std::wstring installer_arguments(const std::wstring& file, const std::wstring& directory) {
    return L"/i " + quote_argument(file) + L" TARGETDIR=" + quote_argument(directory) + L" /passive /norestart";
}

template<class Read, class Write>
bool transfer_body(Read read, Write write, std::uint64_t limit,
    std::optional<std::uint64_t> expected = std::nullopt) {
    if (!limit || (expected && (!*expected || *expected > limit))) return false;
    char buffer[16384];
    std::uint64_t total = 0;
    while (true) {
        std::size_t count = 0;
        if (!read(buffer, sizeof(buffer), count) || count > sizeof(buffer)) return false;
        if (!count) break;
        if (count > limit - total || (expected && count > *expected - total)) return false;
        if (!write(std::string_view(buffer, count))) return false;
        total += count;
    }
    return total > 0 && (!expected || total == *expected);
}

// Production and tests use this same gate. A failed/partial download or failed
// verification must never reach installation. There is no uninstall operation.
template<class Download, class Verify, class Install>
bool perform_update(Download download, Verify verify, Install install) {
    return download() && verify() && install();
}

} // namespace wrt
