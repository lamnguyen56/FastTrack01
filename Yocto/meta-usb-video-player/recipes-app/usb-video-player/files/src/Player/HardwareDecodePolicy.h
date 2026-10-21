#pragma once

/** @brief Restrict the Qt5 GStreamer process to the Pi's hardware H.264 video decoder. */
namespace HardwareDecodePolicy {
/** @brief Apply decoder ranks before creating media sessions; audio decoders remain available. */
void apply();
}
