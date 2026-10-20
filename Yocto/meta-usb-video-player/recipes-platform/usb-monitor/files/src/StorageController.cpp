/**
 * @file StorageController.cpp
 * @brief USB storage-session coordination.
 */

#include "StorageController.h"
#include "Logger.h"

namespace usb {

StorageController::StorageController(const std::string& mountPath, const std::string& playlistPath)
    : mount_(mountPath), playlist_(playlistPath) {
    playlist_.clear();
}

StorageController::~StorageController() {
    release(false);
}

void StorageController::add(const Device& device) {
    if (device.usbPartition == false || activePath_.empty() == false) {
        return;
    }

    // Drain the previous cancelled worker before creating a new storage session.
    if (scanner_.busy() == true) {
        enumerate_ = true;
        return;
    }

    if (device.node.empty() == true || device.sysPath.empty() == true) {
        return;
    }

    if (device.filesystem.empty() == false && device.filesystem != "vfat") {
        return;
    }

    if (mount_.mount(device) == false) {
        return;
    }

    activePath_ = device.sysPath;

    if (scanner_.start(mount_.path()) == false) {
        release(false);
        return;
    }

    LOG_I("[usb] Scanning ", device.node);
}

void StorageController::onDevice(const Event& event) {
    if (event.action == Event::Action::Remove) {
        // Unrelated partition removals must not disturb the selected USB.
        if (activePath_.empty() == false && activePath_ == event.device.sysPath) {
            release(true);
        }
    } else {
        add(event.device);
    }
}

void StorageController::release(bool removed) {
    if (activePath_.empty() == true) {
        return;
    }

    // Invalidate the worker before clearing publication and releasing the mount.
    LOG_I("Releasing USB session: ", activePath_, ", physically removed=", removed);
    scanner_.cancel();
    playlist_.clear();
    mount_.release(removed);
    activePath_.clear();
    enumerate_ = true;
}

void StorageController::poll() {
    const std::optional<ScanResult> result = scanner_.poll();
    if (result.has_value() == false) {
        return;
    }

    // A completed result from removed storage must never restore a stale playlist.
    if (activePath_.empty() == false) {
        if (result->valid == true) {
            LOG_I("USB scan complete: ", result->files.size(), " media files");
            playlist_.write(result->files);
        } else {
            playlist_.clear();
            LOG_E("USB scan failed; playlist cleared");
        }
    } else {
        enumerate_ = true;
    }
}

bool StorageController::takeEnumerationRequest() {
    const bool value = enumerate_;
    enumerate_ = false;

    return value;
}

}
