# Python接收脚本 - 终端使用说明

# 在手机Termux (proot-Debian)中:
# 1. 安装依赖
pip install mido python-rtmidi

# 2. 加载虚拟MIDI内核模块
sudo modprobe snd-virmidi

# 3. 运行接收脚本
python midi_receiver.py --port 5000

# 4. 启动MuseScore并配置ALSA输入
mscore -a alsa
# 在MuseScore的I/O设置中选择虚拟MIDI端口作为输入