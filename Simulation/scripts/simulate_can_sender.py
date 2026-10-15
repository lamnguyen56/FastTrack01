#!/usr/bin/env python3
"""
simulate_can_sender.py
Công cụ giả lập phát frame CAN ID 0x100 qua SocketCAN (can0 hoặc vcan0)
Dùng để kiểm thử nhanh can-daemon và Qt App trên máy dev Linux.

Cài đặt:
    pip install python-can

Sử dụng:
    python simulate_can_sender.py -i can0
    python simulate_can_sender.py -i vcan0 --auto
"""

import sys
import time
import argparse

try:
    import can
except ImportError:
    print("[ERROR] Thư viện 'python-can' chưa được cài đặt. Hãy chạy: pip install python-can")
    sys.exit(1)

CAN_ID_CMD = 0x100
CAN_ID_STATUS = 0x101

COMMAND_MAP = {
    'play': 0x01,
    'pause': 0x02,
    'next': 0x03,
    'prev': 0x04,
    'ff': 0x05,
    'rew': 0x06
}

def create_command_frame(cmd_byte: int, alive_counter: int) -> can.Message:
    data = [cmd_byte, alive_counter & 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00]
    # Checksum = XOR Byte 0..6
    cks = 0
    for b in data:
        cks ^= b
    data.append(cks)
    return can.Message(arbitration_id=CAN_ID_CMD, data=data, is_extended_id=False)

def main():
    parser = argparse.ArgumentParser(description="CAN Command Simulator for UBS Video Player")
    parser.add_argument("-i", "--interface", default="can0", help="CAN interface name (default: can0)")
    parser.add_argument("--auto", action="store_true", help="Automatically send commands periodically")
    args = parser.parse_args()

    print("=" * 60)
    print(f"Khởi động CAN Simulator trên interface: {args.interface}")
    print("=" * 60)

    try:
        bus = can.interface.Bus(channel=args.interface, bustype='socketcan')
    except Exception as e:
        print(f"[ERROR] Không thể mở interface {args.interface}: {e}")
        print("Gợi ý tạo virtual CAN: sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0")
        sys.exit(1)

    alive = 0
    if args.auto:
        print("[INFO] Chế độ AUTO: Đang tự động phát chu kỳ lệnh...")
        commands = ['play', 'ff', 'next', 'pause']
        idx = 0
        while True:
            cmd = commands[idx % len(commands)]
            msg = create_command_frame(COMMAND_MAP[cmd], alive)
            bus.send(msg)
            print(f"[TX] Gửi lệnh: {cmd.upper()} (Data: {[hex(x) for x in msg.data]})")
            alive = (alive + 1) % 16
            idx += 1
            time.sleep(3)
    else:
        print("Nhập lệnh (play, pause, next, prev, ff, rew, quit):")
        while True:
            try:
                line = input("CMD > ").strip().lower()
                if line == 'quit' or line == 'q':
                    break
                if line in COMMAND_MAP:
                    msg = create_command_frame(COMMAND_MAP[line], alive)
                    bus.send(msg)
                    print(f"[TX] Đã gửi {line.upper()} -> Frame CAN ID 0x100: {[hex(x) for x in msg.data]}")
                    alive = (alive + 1) % 16
                else:
                    print("Lệnh không hợp lệ! Chọn: play, pause, next, prev, ff, rew, quit")
            except (KeyboardInterrupt, EOFError):
                break

    bus.shutdown()
    print("\nĐã đóng kết nối CAN.")

if __name__ == '__main__':
    main()
