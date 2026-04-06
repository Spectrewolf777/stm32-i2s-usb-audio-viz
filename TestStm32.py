import sys
import time
import wave
import serial
import numpy as np
import pyqtgraph as pg
import pyqtgraph.exporters
from pyqtgraph.Qt import QtCore, QtWidgets

# Configuration
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 2000000          
RECORD_DURATION = 10.0  

class AudioRecorder:
    def __init__(self):
        self.app = QtWidgets.QApplication(sys.argv)
        self.win = pg.GraphicsLayoutWidget(show=True, title="10-Second Recorder & Audio Exporter")
        self.plot = self.win.addPlot(title="Recording Live...")
        self.curve = self.plot.plot(pen='y')
        
        self.all_data = []
        
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
            print(f"Connected to {SERIAL_PORT}. Recording...")
        except Exception as e:
            print(f"Error: {e}")
            sys.exit()

        self.start_time = time.time()
        self.recording_active = True

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.timer.start(20) 

    def update(self):
        if not self.recording_active:
            return

        current_time = time.time()
        elapsed = current_time - self.start_time

        bytes_available = self.ser.in_waiting
        if bytes_available >= 2:
            bytes_to_read = bytes_available - (bytes_available % 2)
            raw_data = self.ser.read(bytes_to_read)
            new_samples = np.frombuffer(raw_data, dtype=np.int16)
            
            self.all_data.append(new_samples)
            self.curve.setData(new_samples[-2048:])
            self.plot.setTitle(f"Recording... {elapsed:.1f} / {RECORD_DURATION}s")

        if elapsed >= RECORD_DURATION:
            self.recording_active = False
            self.timer.stop()
            self.ser.close()
            print("\nRecording finished! Processing files...")
            self.finish_and_save()

    def finish_and_save(self):
        # 1. Flatten the data
        final_waveform = np.concatenate(self.all_data)
        self.curve.setData(final_waveform)
        self.plot.enableAutoRange('x', True)
        self.plot.enableAutoRange('y', True)
        
        # 2. Calculate the exact sample rate achieved
        total_samples = len(final_waveform)
        sample_rate = int(total_samples / RECORD_DURATION)
        print(f"Detected Sample Rate: {sample_rate} Hz")
        
        # 3. Save the Graph
        try:
            exporter = pg.exporters.ImageExporter(self.plot)
            exporter.parameters()['width'] = 1200 
            exporter.export('final_waveform.png')
            print("💾 Saved image as 'final_waveform.png'")
        except Exception as e:
            print(f"Could not save image: {e}")

        # 4. Amplify and Save the Audio File
        # Let's boost the volume by 20x so quiet noises are audible
        boosted_audio = final_waveform.astype(np.float32) * 20.0
        # Clip it so it doesn't break the int16 boundaries
        boosted_audio = np.clip(boosted_audio, -32768, 32767).astype(np.int16)

        try:
            with wave.open('audio_capture.wav', 'w') as wf:
                wf.setnchannels(1)        # 1 = Mono
                wf.setsampwidth(2)        # 2 bytes = 16-bit
                wf.setframerate(sample_rate)
                wf.writeframes(boosted_audio.tobytes())
            print("🎵 Success! Saved audio as 'audio_capture.wav' in your current folder.")
        except Exception as e:
            print(f"Failed to save audio file: {e}")

    def run(self):
        sys.exit(self.app.exec_())

if __name__ == '__main__':
    recorder = AudioRecorder()
    recorder.run()