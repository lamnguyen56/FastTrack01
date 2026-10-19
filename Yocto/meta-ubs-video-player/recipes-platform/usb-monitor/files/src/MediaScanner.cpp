/**
 * @file MediaScanner.cpp
 * @brief Asynchronous cancellable media discovery.
 */

#include "MediaScanner.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <chrono>
#include <iostream>

namespace usb {

namespace fs = std::filesystem;
ScanResult MediaScanner::scan(const std::string& path, const std::shared_ptr<std::atomic<bool>>& cancel) {
    ScanResult result;

    // Error-code filesystem APIs turn unplug/I/O failures into invalid results.
    std::error_code ec;
    fs::recursive_directory_iterator it(path, fs::directory_options::skip_permission_denied, ec), end;
    if (ec.value() != 0) {
        return result;
    }
    while (it != end) {
        if (cancel->load() == true) {
            return result;
        }
        const fs::directory_entry entry = *it;

        // Do not follow file symlinks outside the selected storage.
        const bool symlink = entry.is_symlink(ec);
        if (ec.value() != 0) {
            return result;
        }
        if (symlink == false && entry.is_regular_file(ec)) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) {
                return std::tolower(c);
            }
            );
            if (ext == ".mp4" || ext == ".mkv" || ext == ".avi" || ext == ".mov" || ext == ".webm") {
                result.files.push_back(entry.path().string());
            }
        }
        if (ec.value() != 0) {
            return result;
        }
        it.increment(ec);
        if (ec.value() != 0) {
            return result;
        }
    }

    if (cancel->load() == true) {
        return result;
    }

    std::sort(result.files.begin(), result.files.end());
    result.valid = true;

    return result;
}

MediaScanner::~MediaScanner() {
    cancel();
    if (job_.valid() == true) {
        job_.wait();
    }
}

bool MediaScanner::start(const std::string& path) {
    // Do not replace a future: destroying it could join a still-running worker.
    if (busy() == true) {
        return false;
    }

    cancel_ = std::make_shared<std::atomic<bool>>(false);
    try {
        job_ = std::async(std::launch::async, &MediaScanner::scan, path, cancel_);
    } catch (const std::exception& e) {
        std::cerr << "[usb] Cannot start scan: " << e.what() << '\n';
        return false;
    }
    return true;
}

void MediaScanner::cancel() {
    if (cancel_ != nullptr) {
        cancel_->store(true);
    }
}

std::optional<ScanResult> MediaScanner::poll() {
    if (busy() == false || job_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return std::nullopt;
    }

    try {
        ScanResult result = job_.get();

        // Cancellation wins even if traversal completed just before removal.
        if (cancel_->load() == true) {
            result.valid = false;
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[usb] Scan error: " << e.what() << '\n';
        return ScanResult{};
    }

}

}
