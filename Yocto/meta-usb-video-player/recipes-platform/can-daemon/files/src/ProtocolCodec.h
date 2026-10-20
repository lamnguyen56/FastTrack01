/**
 * @file ProtocolCodec.h
 * @brief CAN command/status wire-format conversion.
 */

#pragma once

#include "Constants.h"
#include <linux/can.h>

namespace canbus {

/**
 * @brief Convert between Classical CAN payloads and shared IPC packets.
 * @note Stateless codec; no socket operations, duplicate filtering or checksum validation.
 */
class ProtocolCodec {

public:
    /**
     * @brief Extract command fields from a standard data frame with command ID.
     * @param frame Received frame; requires DLC 5 through 8.
     * @param[out] command Decoded payload, written only on success.
     * @return False for wrong ID/flags or an unsupported DLC.
     * @note Command values, counter and checksum are copied, not semantically validated.
     */
    static bool decodeCommand(const can_frame& frame, FastTrack::CanCommandPacket& command);

    /**
     * @brief Encode status with big-endian time fields and configured status ID.
     * @param status Snapshot supplied by the Qt application.
     * @return Zero-initialized eight-byte Classical CAN data frame with status fields.
     */
    static can_frame encodeStatus(const FastTrack::CanStatusPacket& status);
};

}
