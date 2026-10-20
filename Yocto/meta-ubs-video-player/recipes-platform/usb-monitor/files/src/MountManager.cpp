/**
 * @file MountManager.cpp
 * @brief Read-only USB mount ownership and cleanup.
 */

#include "MountManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Logger.h"
#include <cerrno>
#include <cstring>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>

namespace usb {

namespace fs = std::filesystem;

// Return 0 if free, 1 if occupied by this device, -1 if occupied by another
// device or mount information cannot be read. Never stack mounts at this path.
/**
 * @brief Check the mount point and prevent duplicate/system-volume mounting.
 * @param path Fixed mount directory without mountinfo escape characters.
 * @param device Candidate block-device number.
 * @return 0 if free, 1 for this device, -1 for conflicts or unreadable mountinfo.
 */
static int mountedAt(const std::string& path, dev_t device) {
    std::ifstream in("/proc/self/mountinfo");
    if (in.fail() == true) {
        return -1;
    }
    std::string line;
    int result = 0;
    const auto expected = std::to_string(major(device)) + ":" + std::to_string(minor(device));
    while (std::getline(in, line)) {
        std::istringstream fields(line);
        std::string id, parent, number, root, target;
        fields >> id >> parent >> number >> root >> target;
        if (fields.fail() == true) {
            continue;
        }

        // Never adopt a USB root filesystem or a volume mounted elsewhere.
        if (number == expected && target != path) {
            return -1;
        }
        if (target == path) {
            if (result != 0) {
                return -1;
            }
            result = number == expected ? 1 : -1;
        }
    }
    return result;
}

bool MountManager::mount(const Device& device) {
    if (owned_ == true) {
        return false;
    }
    std::error_code ec;
    fs::create_directories(path_, ec);
    if (ec.value() != 0) {
        LOG_E("[usb] mkdir: ", ec.message());
        return false;
    }
    // Never cover existing data or mount a USB root filesystem a second time.
    const int state = mountedAt(path_, device.number);
    if (state < 0) {
        LOG_W("USB mount point unavailable; refusing to replace an existing mount");
        return false;
    }
    // A service restart may adopt its previous mount, but only if still read-only.
    if (state == 1) {
        struct statvfs info{};
        if (::statvfs(path_.c_str(), &info) != 0 || (info.f_flag & ST_RDONLY) == 0) {
            LOG_W("Refusing to adopt read-write USB mount"); return false;
        }

    } else {
        if (fs::is_empty(path_, ec) == false || ec.value() != 0) {
            LOG_E("[usb] Mount directory not empty");
            return false;
        }
        if (::mount(device.node.c_str(), path_.c_str(), "vfat", MS_RDONLY | MS_NODEV | MS_NOSUID | MS_NOEXEC, nullptr) != 0) {
            LOG_E("[usb] mount ", device.node, ": ", std::strerror(errno)); return false;
        }
    }

    if (mountedAt(path_, device.number) != 1) {
        return false;
    }
    device_ = device.number;
    owned_ = true;
    LOG_I("USB mounted read-only: ", device.node, " -> ", path_);

    return true;
}

void MountManager::release(bool physicallyRemoved) {
    if (owned_ == false) {
        return;
    }

    // Match ownership again: never unmount a replacement device at the same path.
    if (mountedAt(path_, device_) == 1 && ::umount2(path_.c_str(), physicallyRemoved ? MNT_DETACH : 0) != 0) {
        LOG_E("[usb] unmount: ", std::strerror(errno));
    }
    owned_ = false;
    device_ = 0;
}

}
