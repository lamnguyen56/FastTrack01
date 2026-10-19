// Test checks must remain enabled even when built with -DNDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../files/src/ProtocolCodec.h"
#include <cassert>
#include <iostream>

int main() {
    can_frame command{};
    command.can_id = FastTrack::CAN_ID_COMMAND;
    command.can_dlc = 5;
    command.data[0] = 1;
    command.data[1] = 5;
    command.data[2] = 10;
    command.data[3] = 7;
    command.data[4] = 23;
    FastTrack::CanCommandPacket decoded{};
    assert(canbus::ProtocolCodec::decodeCommand(command, decoded));
    assert(decoded.commandId == 1 && decoded.seekCommand == 5 && decoded.seekStepValue == 10);
    assert(decoded.rollingCounter == 7 && decoded.checksum == 23);
    command.can_dlc = 4;
    assert(!canbus::ProtocolCodec::decodeCommand(command, decoded));
    command.can_dlc = 5;
    command.can_id |= CAN_RTR_FLAG;
    assert(!canbus::ProtocolCodec::decodeCommand(command, decoded));
    FastTrack::CanStatusPacket status{1, 3, 0x1234, 0x5678, 2, 42};
    const auto frame = canbus::ProtocolCodec::encodeStatus(status);
    assert(frame.can_id == FastTrack::CAN_ID_STATUS && frame.can_dlc == 8);
    assert(frame.data[0] == 1 && frame.data[1] == 3);
    assert(frame.data[2] == 0x12 && frame.data[3] == 0x34);
    assert(frame.data[4] == 0x56 && frame.data[5] == 0x78);
    assert(frame.data[6] == 2 && frame.data[7] == 42);
    std::cout << "PASS: CAN command fields, invalid frame rejection and big-endian status\n";
}
