/**
 * @file main.cpp
 * @brief USB Monitor Daemon - Tầng Platform xử lý cắm/rút USB
 *
 * Chức năng:
 *   1. Lắng nghe sự kiện USB hotplug qua libudev (Netlink kernel socket)
 *   2. Tự động mount phân vùng USB vào /media/usb0
 *   3. Quét đệ quy các file media (.mp4, .mkv, .avi)
 *   4. Ghi danh sách file media ra /tmp/usb_playlist.txt cho Qt App đọc
 *   5. Khi rút USB: xóa playlist, unmount an toàn
 *
 * Giao tiếp với Qt App: Qua file /tmp/usb_playlist.txt (filesystem-based IPC, đơn giản nhất)
 *   - Qt App dùng QFileSystemWatcher theo dõi file này để biết khi nào có USB mới
 */

#include <iostream>
#include <algorithm>
#include <cctype>
#include <sys/select.h>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <libudev.h>
#include "Constants.h"

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
}

static const char* PLAYLIST_FILE = "/tmp/usb_playlist.txt";

// ============================================================================
// Quét đệ quy tìm file media trong mount point
// ============================================================================
static std::vector<std::string> scanMediaFiles(const std::string& mountPath) {
    std::vector<std::string> results;
    namespace fs = std::filesystem;

    if (!fs::exists(mountPath)) return results;

    for (const auto& entry : fs::recursive_directory_iterator(
             mountPath, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        // Chuyển về lowercase
        for (auto& c : ext) c = std::tolower(c);

        if (ext == ".mp4" || ext == ".mkv" || ext == ".avi" || ext == ".mov" || ext == ".webm") {
            results.push_back(entry.path().string());
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

// ============================================================================
// Ghi playlist ra file cho Qt App đọc
// ============================================================================
static void writePlaylist(const std::vector<std::string>& files) {
    std::ofstream ofs(PLAYLIST_FILE, std::ios::trunc);
    for (const auto& f : files) {
        ofs << f << "\n";
    }
    ofs.flush();
    std::cout << "[usb-monitor] Playlist written: " << files.size() << " files -> " << PLAYLIST_FILE << "\n";
}

static void clearPlaylist() {
    std::ofstream ofs(PLAYLIST_FILE, std::ios::trunc);
    ofs.flush();
    std::cout << "[usb-monitor] Playlist cleared.\n";
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=========================================================\n";
    std::cout << " USB Monitor Daemon (Platform Layer)                     \n";
    std::cout << " Mount Point: " << FastTrack::DEFAULT_USB_MOUNT_POINT << "\n";
    std::cout << " Playlist:    " << PLAYLIST_FILE << "                   \n";
    std::cout << "=========================================================\n";

    // Khởi tạo libudev
    struct udev* udev = udev_new();
    if (!udev) {
        std::cerr << "[usb-monitor] FATAL: Cannot create udev context\n";
        return 1;
    }

    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", "partition");
    udev_monitor_enable_receiving(mon);
    int monFd = udev_monitor_get_fd(mon);

    // Kiểm tra trạng thái ban đầu: nếu USB đã được cắm sẵn
    namespace fs = std::filesystem;
    if (fs::exists(FastTrack::DEFAULT_USB_MOUNT_POINT) &&
        !fs::is_empty(FastTrack::DEFAULT_USB_MOUNT_POINT)) {
        std::cout << "[usb-monitor] USB already mounted at startup. Scanning...\n";
        auto files = scanMediaFiles(FastTrack::DEFAULT_USB_MOUNT_POINT);
        writePlaylist(files);
    }

    std::cout << "[usb-monitor] Listening for USB hotplug events...\n";

    while (g_running) {
        // Poll udev monitor fd (non-blocking, timeout 500ms)
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(monFd, &fds);
        struct timeval tv = {0, 500000}; // 500ms

        int ret = select(monFd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(monFd, &fds)) {
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char* action = udev_device_get_action(dev);
                const char* devnode = udev_device_get_devnode(dev);

                if (action && devnode) {
                    std::string act(action);

                    if (act == "add") {
                        std::cout << "[usb-monitor] USB INSERTED: " << devnode << "\n";

                        // Automount (udev-extraconf thường đã mount, nhưng đảm bảo an toàn)
                        std::string mountCmd = std::string("mount ") + devnode + " "
                                             + FastTrack::DEFAULT_USB_MOUNT_POINT + " 2>/dev/null";
                        system(("mkdir -p " + std::string(FastTrack::DEFAULT_USB_MOUNT_POINT)).c_str());
                        system(mountCmd.c_str());

                        // Quét media và ghi playlist
                        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Đợi mount ổn định
                        auto files = scanMediaFiles(FastTrack::DEFAULT_USB_MOUNT_POINT);
                        writePlaylist(files);

                    } else if (act == "remove") {
                        std::cout << "[usb-monitor] USB REMOVED: " << devnode << "\n";
                        clearPlaylist();
                        system(("umount " + std::string(FastTrack::DEFAULT_USB_MOUNT_POINT) + " 2>/dev/null").c_str());
                    }
                }
                udev_device_unref(dev);
            }
        }
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
    clearPlaylist();
    std::cout << "[usb-monitor] Stopped.\n";
    return 0;
}
