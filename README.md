# ESP32 INMP441 Microphone Visualizer

This project runs on an ESP32-S3 and streams microphone audio data for a realtime web spectrogram, VU meter, and waveform display.

A script is also included for streaming audio to MATLAB.

## 1) What You Need

### Hardware

- ESP32-S3 board 
- INMP441 I2S microphone
- USB cable (data cable, not charge-only)
- Wi-Fi network for STA mode (optional; device can fall back to AP mode)

### Software

- Visual Studio Code
- Node.js (includes npm)
- Python 3 (required by PlatformIO toolchain)
- GitHub Desktop
- PlatformIO IDE extension for VS Code

### Documentation

Some documentation to assist is included in the repository:
- [INMP441 datasheet](docs/INMP441.pdf)
- [MATLAB realtime audio client](docs/matlab_realtime_audio_client.m)
- Unexpected Maker ESP32 ProS3 [schematic](docs/schematic-pros3.pdf) and [pin reference](docs/ProS3_Pin_Reference.png)

n.b. you can use any ESP32 board - this one was just one I had laying around

## 2) Install Required Tools

Install these in this order.

1. Install Visual Studio Code
	- Download: https://code.visualstudio.com/

2. Install Node.js LTS (npm is included)
	- Download: https://nodejs.org/
	- Verify in a terminal:

	```bash
	node --version
	npm --version
	```

3. Install Python 3
	- Download: https://www.python.org/downloads/
	- On Windows, make sure Add Python to PATH is enabled during install.
	- Verify:

	```bash
	python --version
	```

4. Install GitHub Desktop
	- Download: https://desktop.github.com/
	- Sign in with your GitHub account in GitHub Desktop.

5. Install PlatformIO IDE extension in VS Code
	- Marketplace: https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide
	- In VS Code, open Extensions and install PlatformIO IDE.

## 3) Download the Code from GitHub (GitHub Desktop)

Repository URL:

https://github.com/dhskinner/inmp441-microphone

1. Open GitHub Desktop.
2. Click File > Clone Repository.
3. Choose URL tab.
4. Paste:

```text
https://github.com/dhskinner/inmp441-microphone
```

5. Choose a local path where you want the project folder created.
6. Click Clone.

## 4) Open Project in VS Code

1. Open VS Code.
2. File > Open Folder.
3. Select this project folder.
4. Wait for PlatformIO to initialize toolchains (first run can take a few minutes).

## 5) Configure Project Settings

### Board settings

The project should run on any ESP32 (currently an Unexpected Maker ProS3) or other microcontrollers that support I2S. 

For setup of alternative boards follow the platform.io guide at https://docs.platformio.org/en/latest/boards/index.html and update pin settings in src/settings.h:

```cpp
#define I2S_WS 38
#define I2S_SD 40
#define I2S_SCK 39
```

### Wi-Fi credentials

Edit src/settings.h and update these values:

```cpp
#define WIFI_SSID "Your-WiFi-Name"
#define WIFI_PASSWORD "Your-WiFi-Password"
```

If Wi-Fi connection fails at runtime, firmware automatically starts fallback AP mode:

```cpp
#define AP_SSID "WhaleBoy"
#define AP_PASSWORD "password01"
```

### Mic channel note

If you get silence, change I2S channel mode in src/settings.h:

```cpp
// Default
#define I2S_CHANNEL_FORMAT AudioMode::LeftChannel

// Try this if you see silence
// #define I2S_CHANNEL_FORMAT AudioMode::RightChannel
```

## 6) Build the Firmware

You can build with either VS Code UI or terminal.

### Option A: VS Code + PlatformIO UI

1. In VS Code, open PlatformIO from the Alien Head icon in the left Activity Bar.
2. In PlatformIO, go to Project Tasks > um_pros3 > General > Build.
3. Quick method: click the checkmark icon in the VS Code status bar to Build.

### Option B: Terminal command

Run from the project root:

```bash
platformio run
```

On this machine, full path is often:

```powershell
C:\Users\<your-user>\.platformio\penv\Scripts\platformio.exe run
```

## 7) Flash to ESP32

1. Connect the ESP32-S3 with a USB data cable.
2. If needed, install USB serial drivers:
	- CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
	- CH34x: https://www.wch.cn/downloads/CH341SER_ZIP.html

Then upload.

### Option A: VS Code + PlatformIO UI

1. Open PlatformIO from the 'Alien Head' icon in the left Activity Bar.
2. Go to Project Tasks > um_pros3 > General > Upload.
3. Quick method: click the right-arrow icon in the VS Code status bar to Upload.

### Option B: Terminal command

```bash
platformio run --target upload
```

If multiple COM ports exist, set upload_port in platformio.ini for your board port, for example:

```ini
upload_port = COM7
```

## 8) Monitor Logs (Recommended)

Use serial monitor after upload to confirm Wi-Fi and server startup.

### Option A: VS Code + PlatformIO UI

1. Open PlatformIO from the 'Alien Head' icon in the left Activity Bar.
2. Go to Project Tasks > um_pros3 > Monitor.
3. Quick method: use the PlatformIO quick-action icons in the VS Code status bar (same area as Build/Upload).

### Option B: Terminal command

```bash
platformio device monitor --baud 115200
```

## 9) Use the Web Client

1. After boot, read serial output for IP address.
2. Open browser to that IP.
3. You should see waveform, spectrogram, and VU meter views.

## 10) MATLAB Setup (Optional)

Use this if you want the standalone MATLAB realtime waveform + spectrogram client in docs/matlab_realtime_audio_client.m.

### Install MATLAB

1. Download MATLAB: https://www.mathworks.com/products/matlab.html
2. Install and sign in with your MathWorks account/license.
3. Recommended: MATLAB R2021a or newer.

### Required MATLAB functionality

- tcpclient support (for TCP audio stream connection)
- spectrogram support

If your MATLAB install is minimal, install or enable required products from the MATLAB Add-On / installer manager (for example Signal Processing Toolbox for spectrogram workflows).

### Run the MATLAB client

1. Make sure firmware is already flashed and running.
2. Find the ESP32 IP from serial logs.
3. Open docs/matlab_realtime_audio_client.m in MATLAB.
4. Update host at the top of the script:

```matlab
host = "192.168.1.11";   % Set this to your ESP32 IP
port = 3333;
```

5. Run the script.
6. A figure opens with realtime waveform and spectrogram.

### MATLAB troubleshooting tips

- Connection timeout:
	- Confirm ESP32 and PC are on the same network.
	- Confirm the IP address is correct.
	- Confirm port 3333 is reachable and not blocked by firewall.
- No/garbled data:
	- Reset board and rerun script.
	- Verify firmware is running and audio stream is active.

## 11) Common First-Time Issues

### Build fails because PlatformIO is not found

- Ensure PlatformIO extension is installed in VS Code.
- Restart VS Code once after install.

### Upload fails: cannot open COM port

- Unplug/replug board.
- Close other serial tools that may lock the port.
- Confirm correct COM port and set upload_port.

### Serial monitor shows no logs

- This project enables USB CDC on boot in platformio.ini.
- Open monitor at 115200 baud.
- Press reset after opening monitor.

### No audio or very low signal

- Recheck INMP441 wiring and power.
- Try switching left/right channel setting.

## 12) Useful Commands Quick Reference

- Build:

```bash
platformio run
```

- Upload:

```bash
platformio run --target upload
```

- Clean:

```bash
platformio run --target clean
```

- Monitor:

```bash
platformio device monitor --baud 115200
```

## 13) Project Notes

- Board and environment are configured in platformio.ini.
- Main runtime settings are in src/settings.h.
- Web UI is embedded in firmware from src/webui.h.

