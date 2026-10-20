/**
 * @file UsbDevice.h
 * @brief Owned hotplug event and device values.
 */

#pragma once

#include <string>
#include <sys/types.h>

namespace usb {

// Owned values: no udev object lifetime escapes the hotplug module.
/**
 * @brief Owned device metadata independent of libudev object lifetime.
 */
struct Device {
    /**
     * @brief sysfs path used to match removal to the active partition.
     */
    std::string sysPath;

    /**
     * @brief Block-device path such as /dev/sda1; may be absent on removal.
     */
    std::string node;

    /**
     * @brief ID_FS_TYPE from udev; empty when not yet identified.
     */
    std::string filesystem;

    /**
     * @brief Kernel major/minor device number for mount ownership checks.
     */
    dev_t number = 0;

    /**
     * @brief True only for partitions with a USB-device ancestor.
     */
    bool usbPartition = false;
};

/**
 * @brief Recognized hotplug action together with a device snapshot.
 */
struct Event {
    /**
     * @brief Event category; Add/Change trigger eligibility checks.
     */
    enum class Action {
        Add,    ///< A partition appeared; evaluate it as a candidate.
        Remove, ///< A partition disappeared; match against the active session.
        Change  ///< Partition metadata changed; retry eligibility if idle.
    } action = Action::Add; ///< Hotplug action associated with this snapshot.
    /**
     * @brief Copied metadata, safe after the udev event is unreferenced.
     */
    Device device;
};

}
