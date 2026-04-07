# stm32-i2s-usb-audio-viz
## Work in progress
A real-time audio processing pipeline using the **STM32H743VIT6**. This project captures audio via an I2S microphone, streams it to a Python interface for real-time graphing, and saves the output as a `.wav` file. This repository is designed as a playground to **test, learn, and visualize Digital Signal Processing (DSP)** in real-time.

## 🛠 My Equipment
* **Microcontroller:** STM32H743VIT6 (WeAct Studio on Aliexpress sells them for cheap that what i am using) (you can modify code to run on other STM32s)
* **Converter:** ADAU7002 (PDM to I2S)
* **Microphone:** PDM Microphone(MD-HRA371-H10-4N)

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
On Linux, you may encounter a `Permission Denied` error when trying to access the USB-C port. 

### The Quick Fix (Temporary)
Run this command every time you plug in your STM32:
```bash
sudo chmod 666 /dev/ttyACM0
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
## Example python script with stm32 connect throough usb c
<img width="2400" height="3600" alt="wav_analysis_raw" src="https://github.com/user-attachments/assets/7542c6d6-4545-4697-b0a5-7ad859e1d01d" />

<img width="2400" height="3600" alt="wav_analysis_bandpass" src="https://github.com/user-attachments/assets/ca43c15e-fdda-4db2-882f-64b942b36a41" />

<img width="567" height="246" alt="image" src="https://github.com/user-attachments/assets/14ddd603-ab1a-40d0-8add-6a3d282dea74" />

bandpass filter because of range of pdm microphone, we know everything else is actualy noise.

You can use this to test DSP and see results using usb c.


   
