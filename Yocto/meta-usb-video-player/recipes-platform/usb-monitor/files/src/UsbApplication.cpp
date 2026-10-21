/**
 * @file UsbApplication.cpp
 * @brief USB daemon composition and event loop.
 */

#include "UsbApplication.h"
#include "HotplugMonitor.h"
#include "StorageController.h"
#include "Constants.h"
#include <csignal>
#include "Logger.h"

namespace usb {

namespace {
/**
 * @brief Signal-safe shutdown flag owned by the application event loop.
 */
volatile std::sig_atomic_t running = 1;

/**
 * @brief Request shutdown from SIGINT/SIGTERM without doing I/O in the handler.
 */
void stop(int) {
    running = 0;
}

/**
 * @brief Feed an initial/reselection snapshot through normal controller eligibility checks.
 * @param hotplug Initialized event source.
 * @param storage Controller that selects at most one candidate.
 */
void enumerate(HotplugMonitor& hotplug, StorageController& storage) {
    for (const auto& device : hotplug.enumerate())
    storage.onDevice(Event{Event::Action::Add, device});
}

}

int UsbApplication::run() {
    running = 1;
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    HotplugMonitor hotplug;
    if (hotplug.initialize() == false) {
        LOG_E("[usb] Cannot initialize udev monitor");
        return 1;
    }
    StorageController storage(FastTrack::DEFAULT_USB_MOUNT_POINT, FastTrack::DEFAULT_USB_PLAYLIST_PATH);

    // Subscribe before enumeration; startup insertions remain queued.
    LOG_I("USB service started; mount path=", FastTrack::DEFAULT_USB_MOUNT_POINT);
    enumerate(hotplug, storage);
    while (running != 0) {
        Event event;
        const auto result = hotplug.wait(event, 100);
        if (result == HotplugMonitor::WaitResult::Error) {
            return 1;
        }
        if (result == HotplugMonitor::WaitResult::EventReady) {
            storage.onDevice(event);
            continue; // Drain removal events before publishing scan results.
        }
        if (result == HotplugMonitor::WaitResult::Interrupted) {
            continue;
        }
        storage.poll();
        if (storage.takeEnumerationRequest() == true) {
            enumerate(hotplug, storage);
        }
    }
    return 0;
}

}
