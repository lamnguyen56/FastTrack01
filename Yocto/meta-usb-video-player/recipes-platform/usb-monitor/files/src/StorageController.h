/**
 * @file StorageController.h
 * @brief USB storage-session coordination.
 */

#pragma once

#include "UsbDevice.h"
#include "MountManager.h"
#include "MediaScanner.h"
#include "PlaylistWriter.h"

namespace usb {

// Coordinates storage lifecycle; no libudev or mount syscalls in this module.
/**
 * @brief Coordinate hotplug, mount, scan and playlist for one USB partition.
 * @note All methods are called by the application event-loop thread.
 */
class StorageController {

public:
    /**
     * @brief Compose storage modules and clear the previous playlist.
     * @param mountPath Fixed USB mount directory.
     * @param playlistPath Output used by the existing Qt playlist watcher.
     */
    StorageController(const std::string& mountPath, const std::string& playlistPath);

    /**
     * @brief Cancel/release active storage; the scanner subsequently joins its worker.
     */
    ~StorageController();

    /**
     * @brief Dispatch add/change events or remove only the matching active partition.
     * @param event Owned event snapshot from the hotplug module.
     */
    void onDevice(const Event& event);

    /**
     * @brief Collect a completed scan and check publication while its session remains active.
     */
    void poll();

    /**
     * @brief Consume a reselection request only after the scanner job has been collected.
     * @return False while the scanner is busy, preserving the request; otherwise consume it.
     */
    bool takeEnumerationRequest();

private:
    /**
     * @brief Select an eligible device if idle; defer selection while an old scan drains.
     * @param device Candidate partition and filesystem metadata.
     */
    void add(const Device& device);

    /**
     * @brief Cancel scanning, clear publication and release the active mount.
     * @param removed Whether the release follows physical removal.
     */
    void release(bool removed);

    /**
     * @brief Active partition sysfs identity; empty means no active session.
     */
    std::string activePath_;

    /**
     * @brief Owner of mount policy and Linux mount operations.
     */
    MountManager mount_;

    /**
     * @brief Owner of the active or draining scan worker.
     */
    MediaScanner scanner_;

    /**
     * @brief Publisher for the current file-based IPC.
     */
    PlaylistWriter playlist_;

    /**
     * @brief Pending request to enumerate devices after cancellation/removal.
     */
    bool enumerate_ = false;
};

}
