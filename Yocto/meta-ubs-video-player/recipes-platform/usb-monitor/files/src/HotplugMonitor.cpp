/**
 * @file HotplugMonitor.cpp
 * @brief USB partition enumeration and hotplug subscription.
 */

#include "HotplugMonitor.h"

#include <libudev.h>
#include <sys/select.h>
#include <cerrno>
#include <cstring>
#include "Logger.h"

namespace usb {
namespace {

/**
 * @brief Copy device properties before the libudev object is released.
 * @param device Borrowed, non-null libudev device.
 * @return Owned metadata with no dependency on the source object's lifetime.
 */
Device describe(udev_device* device) {
    Device description;

    // libudev returns borrowed strings. Copy only properties that exist.
    const char* sysPath = udev_device_get_syspath(device);
    if (sysPath != nullptr) {
        description.sysPath = sysPath;
    }

    const char* deviceNode = udev_device_get_devnode(device);
    if (deviceNode != nullptr) {
        description.node = deviceNode;
    }

    const char* filesystemType = udev_device_get_property_value(device, "ID_FS_TYPE");
    if (filesystemType != nullptr) {
        description.filesystem = filesystemType;
    }

    description.number = udev_device_get_devnum(device);

    // A block partition is eligible only when its ancestry contains a USB device.
    const char* deviceType = udev_device_get_devtype(device);
    if (deviceType != nullptr && std::strcmp(deviceType, "partition") == 0) {
        udev_device* usbParent = udev_device_get_parent_with_subsystem_devtype(
        device, "usb", "usb_device");
        description.usbPartition = (usbParent != nullptr);
    }

    return description;
}

} // namespace

HotplugMonitor::~HotplugMonitor() {
    // Release the child subscription before its parent context.
    if (monitor_ != nullptr) {
        udev_monitor_unref(monitor_);
    }

    if (context_ != nullptr) {
        udev_unref(context_);
    }
}

bool HotplugMonitor::initialize() {
    context_ = udev_new();
    if (context_ == nullptr) {
        return false;
    }

    // Processed udev events can contain filesystem metadata unavailable in raw uevents.
    monitor_ = udev_monitor_new_from_netlink(context_, "udev");
    if (monitor_ == nullptr) {
        return false;
    }

    const int filterStatus = udev_monitor_filter_add_match_subsystem_devtype(
    monitor_, "block", "partition");
    if (filterStatus < 0) {
        return false;
    }

    const int subscriptionStatus = udev_monitor_enable_receiving(monitor_);
    if (subscriptionStatus < 0) {
        return false;
    }

    LOG_I("USB hotplug subscription initialized");
    fd_ = udev_monitor_get_fd(monitor_);
    return fd_ >= 0 && fd_ < FD_SETSIZE;
}

std::vector<Device> HotplugMonitor::enumerate() const {
    std::vector<Device> devices;

    if (context_ == nullptr) {
        return devices;
    }

    udev_enumerate* enumeration = udev_enumerate_new(context_);
    if (enumeration == nullptr) {
        return devices;
    }

    // First select block devices, then ask libudev to collect their sysfs entries.
    const int filterStatus = udev_enumerate_add_match_subsystem(enumeration, "block");
    if (filterStatus < 0) {
        LOG_E("[usb] Enumeration filter failed");
        udev_enumerate_unref(enumeration);
        return devices;
    }

    const int scanStatus = udev_enumerate_scan_devices(enumeration);
    if (scanStatus < 0) {
        LOG_E("[usb] Device enumeration failed");
        udev_enumerate_unref(enumeration);
        return devices;
    }

    // Use an explicit linked-list traversal instead of libudev's foreach macro.
    // Entry names are sysfs paths, not /dev device-node paths.
    for (udev_list_entry* entry = udev_enumerate_get_list_entry(enumeration);
    entry != nullptr;
    entry = udev_list_entry_get_next(entry)) {
        const char* sysPath = udev_list_entry_get_name(entry);
        udev_device* device = udev_device_new_from_syspath(context_, sysPath);

        if (device != nullptr) {
            devices.push_back(describe(device));
            udev_device_unref(device);
        }
    }

    LOG_D("Startup/reselection enumeration: ", devices.size(), " block devices");
    udev_enumerate_unref(enumeration);
    return devices;
}

HotplugMonitor::WaitResult HotplugMonitor::wait(Event& event, int timeoutMs) {
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(fd_, &descriptors);

    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    // A bounded wait lets the application handle shutdown and finished scan jobs.
    const int readyCount = select(fd_ + 1, &descriptors, nullptr, nullptr, &timeout);
    if (readyCount < 0) {
        if (errno == EINTR) {
            return WaitResult::Interrupted;
        }
        return WaitResult::Error;
    }

    if (readyCount == 0) {
        return WaitResult::Timeout;
    }

    udev_device* device = udev_monitor_receive_device(monitor_);
    if (device == nullptr) {
        return WaitResult::Interrupted;
    }

    const char* action = udev_device_get_action(device);
    if (action == nullptr) {
        udev_device_unref(device);
        return WaitResult::Interrupted;
    }

    // Decode string actions explicitly; strcmp returns zero for equal strings.
    if (std::strcmp(action, "remove") == 0) {
        event.action = Event::Action::Remove;
    } else if (std::strcmp(action, "change") == 0) {
        event.action = Event::Action::Change;
    } else if (std::strcmp(action, "add") == 0) {
        event.action = Event::Action::Add;
    } else {
        udev_device_unref(device);
        return WaitResult::Interrupted;
    }

    // Copy values while the event is alive; callers never receive borrowed pointers.
    event.device = describe(device);
    LOG_D("Hotplug action=", action, " node=", event.device.node, " sysfs=", event.device.sysPath);
    udev_device_unref(device);
    return WaitResult::EventReady;
}

} // namespace usb
