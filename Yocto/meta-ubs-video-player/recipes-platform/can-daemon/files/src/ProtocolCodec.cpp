/**
 * @file ProtocolCodec.cpp
 * @brief CAN command/status wire-format conversion.
 */

#include "ProtocolCodec.h"

namespace canbus {

bool ProtocolCodec::decodeCommand(const can_frame& frame, FastTrack::CanCommandPacket& command) {
    // Preserve existing wire format; counter/checksum policy is not changed here.
    if (frame.can_id != FastTrack::CAN_ID_COMMAND || frame.can_dlc < 5 || frame.can_dlc > 8) {
        return false;
    }
    command.commandId = frame.data[0];
    command.seekCommand = frame.data[1];
    command.seekStepValue = frame.data[2];
    command.rollingCounter = frame.data[3];
    command.checksum = frame.data[4];

    return true;
}

can_frame ProtocolCodec::encodeStatus(const FastTrack::CanStatusPacket& status) {
    can_frame frame{};
    frame.can_id = FastTrack::CAN_ID_STATUS;
    frame.can_dlc = FastTrack::CAN_DLC;
    frame.data[0] = status.playbackState;
    frame.data[1] = status.trackIndex;

    // Explicit byte order avoids depending on host endianness or packed struct layout.
    frame.data[2] = (status.curTimeSec >> 8) & 0xff;
    frame.data[3] = status.curTimeSec & 0xff;
    frame.data[4] = (status.totalDurationSec >> 8) & 0xff;
    frame.data[5] = status.totalDurationSec & 0xff;
    frame.data[6] = status.usbHealth;
    frame.data[7] = status.aliveCounter;

    return frame;
}

}
