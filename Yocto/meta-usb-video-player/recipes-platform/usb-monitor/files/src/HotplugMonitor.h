/**
 * @file HotplugMonitor.h
 * @brief USB partition enumeration and hotplug subscription.
 */

#pragma once

#include "UsbDevice.h"
#include <vector>

struct udev;
struct udev_monitor;

namespace usb {

/**
 * @brief Owns the libudev subscription and converts events into owned Device values.
 * @note Use initialize() once, then call enumerate()/wait() from the event-loop thread.
 */
class HotplugMonitor {

public:
    /**
     * @brief Outcome of a bounded wait; Interrupted does not represent a device event.
     */
    enum class WaitResult {
        EventReady, ///< A recognized event was copied into the output argument.
        Timeout,    ///< No event arrived before the deadline; poll other modules.
        Interrupted, ///< Signal, unavailable message or unrecognized action; no output event.
        Error       ///< Fatal wait failure; application should exit for service recovery.
    };

    /**
     * @brief Create an inactive monitor without opening udev resources.
     */
    HotplugMonitor() = default;

    /**
     * @brief Release the monitor before its parent udev context.
     */
    ~HotplugMonitor();

    /**
     * @brief Disallow copying owned udev resources.
     */
    HotplugMonitor(const HotplugMonitor&) = delete;

    /**
     * @brief Disallow assigning owned udev resources.
     */
    HotplugMonitor& operator=(const HotplugMonitor&) = delete;

    /**
     * @brief Subscribe to block-partition events before the startup enumeration.
     * @return True when the context, subscription and selectable descriptor are ready.
     * @pre Call only once per instance.
     */
    bool initialize();

    /**
     * @brief Snapshot currently present block devices for startup or reselection.
     * @return Device values; the controller filters eligible USB partitions.
     * @note An empty result can also indicate an enumeration failure; failures are logged.
     */
    std::vector<Device> enumerate() const;

    /**
     * @brief Wait for one recognized hotplug event.
     * @param[out] event Set only when EventReady is returned.
     * @param timeoutMs Nonnegative maximum wait in milliseconds.
     * @return EventReady, Timeout, Interrupted, or Error.
     * @pre initialize() succeeded.
     */
    WaitResult wait(Event& event, int timeoutMs);

private:
    /**
     * @brief Owned libudev context; null before initialization.
     */
    udev* context_ = nullptr;

    /**
     * @brief Owned subscription for block-partition udev events.
     */
    udev_monitor* monitor_ = nullptr;

    /**
     * @brief Borrowed monitor descriptor; owned and closed by libudev.
     */
    int fd_ = -1;
};

}
