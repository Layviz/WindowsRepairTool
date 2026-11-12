#pragma once

#include "windows_support.h"
#include "update.h"
#include <winhttp.h>
#include <wintrust.h>
#include <softpub.h>
#include <sddl.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

namespace wrt {

class internet_handle {
public:
    explicit internet_handle(HINTERNET value) : value_(value) {}
    ~internet_handle() { if (value_) WinHttpCloseHandle(value_); }
    internet_handle(const internet_handle&) = delete;
    internet_handle& operator=(const internet_handle&) = delete;
    HINTERNET get() const { return value_; }
    explicit operator bool() const { return value_ != nullptr; }
private:
    HINTERNET value_;
};

inline bool http_get(const wchar_t* host, const std::wstring& path,
    const std::function<bool(std::string_view)>& write, std::uint64_t limit,
    std::optional<std::uint64_t> expected = std::nullopt) {
    internet_handle session(WinHttpOpen(L"Windows Repair Tool", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session || !WinHttpSetTimeouts(session.get(), 10000, 10000, 10000, 15000)) return false;
    internet_handle connection(WinHttpConnect(session.get(), host, INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection) return false;
    internet_handle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request) return false;
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)) ||
        !WinHttpSendRequest(request.get(), L"Accept: application/vnd.github+json\r\n", static_cast<DWORD>(-1L),
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request.get(), nullptr)) return false;
    DWORD status = 0, status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) || status != 200) return false;
    return transfer_body([&](char* buffer, std::size_t capacity, std::size_t& count) {
        DWORD received = 0;
        const BOOL success = WinHttpReadData(request.get(), buffer, static_cast<DWORD>(capacity), &received);
        count = received;
        return success != FALSE;
    }, write, limit, expected);
}

// Trust both signatures and require the very same signing certificate. An
// unsigned development build must never become an elevated download executor.
inline std::vector<BYTE> trusted_signer(const std::filesystem::path& file) {
    if (!file.is_absolute()) return {};
    WINTRUST_FILE_INFO info{};
    info.cbStruct = sizeof(info);
    info.pcwszFilePath = file.c_str();
    WINTRUST_DATA data{};
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &info;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    data.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | WTD_DISABLE_MD2_MD4;
    GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const LONG status = WinVerifyTrust(nullptr, &policy, &data);
    struct trust_guard {
        GUID& policy;
        WINTRUST_DATA& data;
        ~trust_guard() {
            data.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(nullptr, &policy, &data);
        }
    } guard{policy, data};
    std::vector<BYTE> certificate;
    if (status == ERROR_SUCCESS) {
        const auto provider = WTHelperProvDataFromStateData(data.hWVTStateData);
        const auto signer = provider ? WTHelperGetProvSignerFromChain(provider, 0, FALSE, 0) : nullptr;
        if (signer && signer->csCertChain && signer->pasCertChain[0].pCert) {
            const auto cert = signer->pasCertChain[0].pCert;
            certificate.assign(cert->pbCertEncoded, cert->pbCertEncoded + cert->cbCertEncoded);
        }
    }
    return certificate;
}

class installer_download {
public:
    ~installer_download() {
        // Only remove our own payload and unique directory, never *.msi.
        if (!file_.empty()) DeleteFileW(file_.c_str());
        if (!directory_.empty()) RemoveDirectoryW(directory_.c_str());
    }
    installer_download() = default;
    installer_download(const installer_download&) = delete;
    installer_download& operator=(const installer_download&) = delete;
    const std::filesystem::path& file() const { return file_; }

    bool download(const release_asset& asset) {
        if (!directory_.empty()) return false;
        PWSTR parent = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &parent))) return false;
        std::filesystem::path root(parent);
        CoTaskMemFree(parent);
        GUID id{};
        wchar_t name[40] = {};
        if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, name, 40)) return false;
        const auto directory = root / (std::wstring(L"WRT-Update-") + name);
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"O:BAG:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)", SDDL_REVISION_1, &descriptor, nullptr)) return false;
        struct descriptor_guard {
            PSECURITY_DESCRIPTOR descriptor;
            ~descriptor_guard() { LocalFree(descriptor); }
        } guard{descriptor};
        SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
        const BOOL created = CreateDirectoryW(directory.c_str(), &security);
        if (!created) return false;
        directory_ = directory;
        file_ = directory_ / L"installer.msi";
        unique_handle file(CreateFileW(file_.c_str(), GENERIC_WRITE, 0, &security, CREATE_NEW,
            FILE_ATTRIBUTE_TEMPORARY, nullptr));
        if (!file) return false;
        const std::wstring path(asset.request_path.begin(), asset.request_path.end());
        return http_get(L"github.com", path, [&](std::string_view bytes) {
            DWORD written = 0;
            return WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
                && written == bytes.size();
        }, 512 * 1024 * 1024, asset.size) && FlushFileBuffers(file.get());
    }
private:
    std::filesystem::path directory_, file_;
};

} // namespace wrt
