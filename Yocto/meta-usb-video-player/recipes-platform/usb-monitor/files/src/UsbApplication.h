/**
 * @file UsbApplication.h
 * @brief USB daemon composition and event loop.
 */

#pragma once
namespace usb {

/**
 * @brief Compose USB software blocks and run their event loop.
 */
class UsbApplication {

public:
    /**
     * @brief Subscribe, enumerate startup devices and process events until shutdown.
     * @return Zero on orderly termination, one on hotplug initialization/wait failure.
     */
    int run();
};

}
