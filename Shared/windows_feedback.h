#pragma once

#include "windows_issues.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace wrt {

class taskbar_progress {
public:
    taskbar_progress() : initialized_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)), window_(GetConsoleWindow()) {
        if ((!SUCCEEDED(initialized_) && initialized_ != RPC_E_CHANGED_MODE) || !window_ || !IsWindowVisible(window_)) return;
        if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList3, reinterpret_cast<void**>(&taskbar_))) && FAILED(taskbar_->HrInit())) {
            taskbar_->Release();
            taskbar_ = nullptr;
        }
    }
    ~taskbar_progress() {
        clear();
        if (taskbar_) taskbar_->Release();
        if (SUCCEEDED(initialized_)) CoUninitialize();
    }
    taskbar_progress(const taskbar_progress&) = delete;
    taskbar_progress& operator=(const taskbar_progress&) = delete;
    void update(std::size_t step, std::size_t total, unsigned percent = 0) {
        if (!taskbar_) return;
        taskbar_->SetProgressState(window_, TBPF_NORMAL);
        taskbar_->SetProgressValue(window_, overall_progress(step, total, percent), 100);
    }
    void error() { if (taskbar_) taskbar_->SetProgressState(window_, TBPF_ERROR); }
    void clear() { if (taskbar_) taskbar_->SetProgressState(window_, TBPF_NOPROGRESS); }
private:
    HRESULT initialized_;
    HWND window_;
    ITaskbarList3* taskbar_ = nullptr;
};

// IUserNotification supplies its own modal message loop. Run it off the repair
// thread: notifications must never block a repair or require a tray-window pump.
class desktop_notifications {
public:
    desktop_notifications() : wake_(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {
        if (wake_) {
            try { worker_ = std::thread([this] { work(); }); }
            catch (const std::system_error&) { /* Notifications must not prevent repairs. */ }
        }
    }
    ~desktop_notifications() {
        if (worker_.joinable()) {
            stopping_.store(true);
            SetEvent(wake_.get());
            worker_.join();
        }
    }
    desktop_notifications(const desktop_notifications&) = delete;
    desktop_notifications& operator=(const desktop_notifications&) = delete;
    void show(std::wstring text) {
        if (!worker_.joinable()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_.push_back(std::move(text));
        }
        SetEvent(wake_.get());
    }
private:
    void work() noexcept {
        const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialized)) return;
        IUserNotification* notification = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_UserNotification, nullptr, CLSCTX_INPROC_SERVER,
            IID_IUserNotification, reinterpret_cast<void**>(&notification)))) {
            notification->SetIconInfo(LoadIconW(nullptr, IDI_APPLICATION), L"Windows Repair Tool");
            notification->SetBalloonRetry(5000, 0, 0);
            try {
                for (;;) {
                    if (WaitForSingleObject(wake_.get(), INFINITE) != WAIT_OBJECT_0) break;
                    std::vector<std::wstring> messages;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        messages.swap(pending_);
                    }
                    for (const auto& message : messages) {
                        if (SUCCEEDED(notification->SetBalloonInfo(L"Windows Repair Tool", message.c_str(), NIIF_INFO)))
                            notification->Show(nullptr, 500);
                    }
                    if (stopping_.load()) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        if (pending_.empty()) break; // Drain the completion notification before exiting.
                    }
                }
            } catch (...) { /* Notifications are best-effort. */ }
            notification->Release();
        }
        CoUninitialize();
    }
    unique_handle wake_;
    std::atomic<bool> stopping_{false};
    std::mutex mutex_;
    std::vector<std::wstring> pending_;
    std::thread worker_;
};

} // namespace wrt
