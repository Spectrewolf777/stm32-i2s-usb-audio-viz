# stm32-i2s-usb-audio-viz
## Work in progress
A real-time audio processing pipeline using the **STM32H743VIT6**. This project captures audio via an I2S microphone, streams it to a Python interface for real-time graphing, and saves the output as a `.wav` file. This repository is designed as a playground to **test, learn, and visualize Digital Signal Processing (DSP)** in real-time.

## 🛠 Equipment
* **Microcontroller:** STM32H743VIT6
* **Converter:** ADAU7002 (PDM to I2S)
* **Microphone:** PDM Microphone

> [!TIP]
> **Hardware Note:** You can simplify this setup by using a native **I2S Microphone** (like the SPH0645) to completely skip the ADAU7002 and PDM microphone.

## 🚀 Features
* **Real-time Visualization:** Live graphing of audio waveforms using Python.
* **Storage:** Automatic export to `.wav` format.
* **High Performance:** Leverages the H7 series DSP capabilities.

## 🔌 Connectivity & Compatibility
* **USB-C Interface:** Connect the STM32H743VIT6 to your PC via the USB-C port. The board acts as a **USB CDC (Virtual COM Port)**, streaming raw 16-bit PCM data directly to your machine.
* **Platform Support:** * **Windows:** NOTE: Untested, this has only been tested on linux.
    * **Linux:** Fully supported (appears as `/dev/ttyACM0`).

---
## 🐧 Linux Permissions
On Linux, you may encounter a `Permission Denied` error when trying to access the USB-C port. You have two ways to fix this:

### Option 1: The Quick Fix (Temporary)
Run this command every time you plug in your STM32:
```bash
sudo chmod 666 /dev/ttyACM0
```
### Option 2: The Permanent Solution
```bash
sudo usermod -a -G dialout $USER
```
## 🐍 Python Script Features
repository includes `TestStm32.py`, which handles the heavy lifting on the PC side:
* **High-FPS Graphing:** Uses `pyqtgraph` for lightning-fast, smooth waveform rendering.
* **Auto-Stop:** Automatically records for exactly 10 seconds and stops.

## 📦 Python Dependencies
To run the visualization and recording script, install the following libraries:
```bash
pip install numpy pyqtgraph pyserial PyQt5
```
## 🏃‍♂️ How to Run
1. **Connect the Hardware:** Plug your STM32 board into your computer using a USB-C cable.
2. **Verify Port:** Check that your device is mapped to `/dev/ttyACM0` (on Linux). If you are on Windows or using a different port, update the `SERIAL_PORT` variable at the top of `TestStm32.py` to match your system (e.g., `COM3`).
3. **Execute the Script:**
   ```bash
   python TestStm32.py
   ```
## What happens??
The script will record for exactly 10 seconds. Once the timer hits the limit, it will stop the stream, automatically process the data, and output two files directly into your project directory:

🖼️ final_waveform.png - A high-resolution image export of your recorded waveform.

🎵 audio_capture.wav - The amplified (20x) recorded audio file, mapped to the correct dynamic sample rate.
##Example<img width="1200" height="891" alt="final_waveform" src="https://github.com/user-attachments/assets/06151c89-9002-4617-8a9e-a8e0213f3573" />
<img width="332" height="120" alt="image" src="https://github.com/user-attachments/assets/4b64fda2-c4c3-48fb-a04e-2042a60e8eee" />

You will be able to see your dsp through usb c using python.


   
