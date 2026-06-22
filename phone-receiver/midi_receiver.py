#!/usr/bin/env python3
"""
ESP32-S3 MIDI Gateway - Phone-side UDP Receiver
接收ESP32转发的UDP MIDI数据包，通过ALSA虚拟MIDI端口注入MuseScore

依赖:
  pip install python-rtmidi mido

使用:
  python midi_receiver.py [--port 5000] [--alsa-port 14:0]
"""
import argparse
import socket
import struct
import threading
import time
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
logger = logging.getLogger('MIDI-RX')

MIDI_PACKET_FORMAT = '<IBBBB'
MIDI_PACKET_SIZE = struct.calcsize(MIDI_PACKET_FORMAT)


def parse_midi_packet(data: bytes) -> dict:
    """解析从ESP32接收的MIDI数据包"""
    if len(data) < MIDI_PACKET_SIZE:
        return None
    timestamp, status, data1, data2, channel = struct.unpack(MIDI_PACKET_FORMAT, data[:MIDI_PACKET_SIZE])
    return {
        'timestamp': timestamp,
        'status': status,
        'data1': data1,
        'data2': data2,
        'channel': channel,
    }


def midi_event_to_mido(msg_dict: dict):
    """将MIDI数据包转换为mido消息"""
    status = msg_dict['status']
    channel = msg_dict['channel']
    data1 = msg_dict['data1']
    data2 = msg_dict['data2']
    
    status_high = status & 0xF0
    msg_type = {
        0x80: 'note_off',
        0x90: 'note_on',
        0xA0: 'polyphonic_key_pressure',
        0xB0: 'control_change',
        0xC0: 'program_change',
        0xD0: 'channel_pressure',
        0xE0: 'pitch_wheel',
    }.get(status_high)
    
    if not msg_type:
        logger.warning(f'Unknown MIDI status: 0x{status:02X}')
        return None
    
    try:
        import mido
        if msg_type in ('program_change', 'channel_pressure'):
            msg = mido.Message(msg_type, channel=channel, value=data1)
        elif msg_type == 'pitch_wheel':
            pitch = (data2 << 7) | data1
            msg = mido.Message(msg_type, channel=channel, pitch=pitch)
        else:
            msg = mido.Message(msg_type, channel=channel, note=data1, velocity=data2)
        return msg
    except ImportError:
        return None


def udp_listener(port: int, callback):
    """UDP监听线程"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', port))
    logger.info(f'UDP listener started on port {port}')
    
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            if callback:
                callback(data, addr)
        except Exception as e:
            logger.error(f'UDP receive error: {e}')


class ALSAMIDIOutput:
    """ALSA虚拟MIDI端口输出"""
    
    def __init__(self, port_name: str = 'ESP32-MIDI'):
        self.port_name = port_name
        self.midiout = None
        self._init_output()
    
    def _init_output(self):
        try:
            import mido
            available_ports = mido.get_output_names()
            logger.info(f'Available MIDI output ports: {available_ports}')
            
            alsa_port = None
            for p in available_ports:
                if 'virtual' in p.lower() or 'Midi Through' in p:
                    alsa_port = p
                    break
            
            if alsa_port:
                self.midiout = mido.open_output(alsa_port)
                logger.info(f'Opened MIDI output: {alsa_port}')
            else:
                logger.warning('No virtual MIDI port found.')
                logger.warning('Create one with: sudo modprobe snd-virmidi')
                logger.warning('Then connect: aconnect 20:0 14:0')
                if available_ports:
                    self.midiout = mido.open_output(available_ports[0])
                    logger.info(f'Opened: {available_ports[0]}')
        except ImportError:
            logger.error('mido library not installed. Install with: pip install mido python-rtmidi')
        except Exception as e:
            logger.error(f'Failed to open MIDI output: {e}')
    
    def send(self, msg_dict: dict):
        if self.midiout is None:
            return
        msg = midi_event_to_mido(msg_dict)
        if msg:
            try:
                self.midiout.send(msg)
            except Exception as e:
                logger.error(f'MIDI send error: {e}')
    
    def close(self):
        if self.midiout:
            self.midiout.close()


def main():
    parser = argparse.ArgumentParser(description='ESP32-S3 MIDI Gateway Receiver')
    parser.add_argument('--port', type=int, default=5000, help='UDP listen port (default: 5000)')
    parser.add_argument('--alsa-port', type=str, default=None, help='ALSA port (e.g., 14:0)')
    args = parser.parse_args()
    
    logger.info('=== ESP32-S3 MIDI Gateway Receiver ===')
    logger.info(f'Listening on UDP port {args.port}')
    
    midi_output = ALSAMIDIOutput()
    
    def on_midi_data(data: bytes, addr):
        msg_dict = parse_midi_packet(data)
        if msg_dict:
            midi_output.send(msg_dict)
    
    listener_thread = threading.Thread(target=udp_listener, args=(args.port, on_midi_data), daemon=True)
    listener_thread.start()
    
    logger.info('Receiver running. Press Ctrl+C to stop.')
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info('Shutting down...')
    finally:
        midi_output.close()


if __name__ == '__main__':
    main()