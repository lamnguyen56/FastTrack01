/**
 * @file MountManager.h
 * @brief Read-only USB mount ownership and cleanup.
 */

#pragma once

#include "UsbDevice.h"
#include <string>
#include <utility>

namespace usb {

/**
 * @brief Manage one FAT32 mount with ownership and read-only checks.
 * @note The controller must call release() before discarding an active manager.
 */
class MountManager {

public:
    /**
     * @brief Select the fixed mount point.
     * @param path Directory reserved for this service; must not contain mountinfo escape characters.
     */
    explicit MountManager(std::string path) : path_(std::move(path)) {}
    /**
     * @brief Mount or adopt the selected device read-only at the configured path.
     * @param device Candidate USB partition already filtered by the controller.
     * @return True only if mount ownership can be verified.
     * @note Refuses occupied paths, hidden files, read-write adoption and devices mounted elsewhere.
     * @pre Process has permission to mount, normally CAP_SYS_ADMIN/root.
     */
    bool mount(const Device& device);

    /**
     * @brief Release only the tracked device at this mount point.
     * @param physicallyRemoved True for unplug cleanup using lazy detach; false for normal unmount.
     * @note Failed normal unmount is logged, not forced; it can leave a busy mount present.
     */
    void release(bool physicallyRemoved);

    /**
     * @brief Return the configured mount path; reference remains valid for this instance.
     */
    const std::string& path() const {
        return path_;
    }

private:
    /**
     * @brief Fixed mount directory used by ownership checks.
     */
    std::string path_;

    /**
     * @brief Tracked block-device number, meaningful only while owned_ is true.
     */
    dev_t device_ = 0;

    /**
     * @brief Whether this instance has accepted responsibility for the mount.
     */
    bool owned_ = false;
};

}
