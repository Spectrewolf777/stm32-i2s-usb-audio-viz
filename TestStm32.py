import os
import sys
import time
import wave
import serial
import numpy as np
import matplotlib
matplotlib.use('Agg') 

import matplotlib.pyplot as plt
from scipy import signal
from scipy.io import wavfile
import pyqtgraph as pg
import pyqtgraph.exporters
from pyqtgraph.Qt import QtCore, QtWidgets

# ==========================================
# CONFIGURATION
# ==========================================
SERIAL_PORT = '/dev/ttyACM0'  # Windows: 'COM3', 'COM4', etc.
BAUD_RATE = 2000000          
RECORD_DURATION = 10.0  

# ==========================================
# POST-ANALYSIS FUNCTION
# ==========================================
def analyze_wav(wav_filename, output_png='wav_analysis.png'):
    """Saves a 4-panel plot: Waveform, Linear FFT, Log (dB) FFT, and Spectrogram."""
    if not os.path.exists(wav_filename):
        return

    sample_rate, data = wavfile.read(wav_filename)
    if len(data.shape) > 1:
        data = data[:, 0]
        
    duration = len(data) / sample_rate
    nyquist = sample_rate / 2
    
    # Setup Figure with 4 Subplots
    fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(12, 18))
    fig.suptitle(f"Deep Audio Analysis: {wav_filename}", fontsize=16, fontweight='bold')

    # --- 1. Time Domain ---
    time_axis = np.linspace(0, duration, len(data))
    ax1.plot(time_axis, data, color='royalblue', alpha=0.7)
    ax1.set_title("Time Domain (Waveform)")
    ax1.set_ylabel("Amplitude")
    ax1.grid(True, alpha=0.3)

    # --- 2. Linear Frequency Domain & Peak Tracking ---
    # Use Blackman-Harris window for superior side-lobe suppression in THD calculation
    window = signal.windows.blackmanharris(len(data))
    fft_spectrum = np.fft.rfft(data * window)
    fft_mag = np.abs(fft_spectrum)
    freq_axis = np.fft.rfftfreq(len(data), 1.0 / sample_rate)
    
    # Peak Detection
    peak_idx = np.argmax(fft_mag[1:]) + 1 # Skip DC
    peak_freq = freq_axis[peak_idx]
    
    ax2.plot(freq_axis, fft_mag, color='orange', alpha=0.8)
    ax2.axvline(peak_freq, color='red', linestyle='--', alpha=0.6, label=f"Peak: {peak_freq:.1f}Hz")
    ax2.set_title(f"Linear Frequency (Primary Peak: {peak_freq:.1f} Hz)")
    ax2.set_ylabel("Magnitude")
    ax2.legend()
    ax2.set_xlim(0, nyquist)
    ax2.grid(True, alpha=0.3)

    # --- 3. Logarithmic Frequency Domain (dBFS) ---
    ref_level = (32768 * len(data)) / 2
    fft_db = 20 * np.log10(np.maximum(fft_mag, 1e-10) / ref_level)
    ax3.plot(freq_axis, fft_db, color='crimson', alpha=0.8)
    ax3.set_title("Logarithmic Frequency (dBFS)")
    ax3.set_ylabel("dB")
    ax3.set_ylim(-120, 0)
    ax3.set_xlim(0, nyquist)
    ax3.grid(True, linestyle='--', alpha=0.5)

    # --- 4. Spectrogram (Heatmap) ---
    f_spec, t_spec, Sxx = signal.spectrogram(data, sample_rate, nperseg=1024)
    Sxx_db = 10 * np.log10(Sxx + 1e-10)
    
    im = ax4.pcolormesh(t_spec, f_spec, Sxx_db, shading='gouraud', cmap='magma')
    ax4.set_title("Spectrogram (Frequency over Time)")
    ax4.set_ylabel("Frequency (Hz)")
    ax4.set_xlabel("Time (s)")
    ax4.set_ylim(0, nyquist)
    fig.colorbar(im, ax=ax4, label='Intensity (dB)')

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(output_png, dpi=200)
    plt.close()
    
    # --- Accurate THD Calculation ---
    def get_power_at_freq(target_freq, search_width=5):
        # Find closest bin and integrate power around it to account for spectral leakage
        idx = np.argmin(np.abs(freq_axis - target_freq))
        if idx == 0: return 0
        start = max(1, idx - search_width)
        end = min(len(fft_mag), idx + search_width + 1)
        return np.sum(fft_mag[start:end]**2)

    fundamental_power = get_power_at_freq(peak_freq)
    
    harmonic_power = 0
    # Calculate power for the 2nd through 10th harmonics
    for i in range(2, 11):
        target_freq = peak_freq * i
        if target_freq > nyquist:
            break
        harmonic_power += get_power_at_freq(target_freq)
        
    if fundamental_power > 0:
        thd = np.sqrt(harmonic_power / fundamental_power) * 100
    else:
        thd = 0.0
        
    print(f"📊 {wav_filename: <30} | True THD: {thd:.4f}% | Dominant Freq: {peak_freq:.1f} Hz")

# ==========================================
# AUDIO RECORDER CLASS
# ==========================================
class AudioRecorder:
    def __init__(self):
        self.app = QtWidgets.QApplication(sys.argv)
        self.win = pg.GraphicsLayoutWidget(show=True, title="Live Diagnostic Monitor")
        self.win.resize(1000, 700)
        
        self.plot_time = self.win.addPlot(row=0, col=0, title="Live Waveform")
        self.curve_time = self.plot_time.plot(pen='y')
        
        self.plot_fft = self.win.addPlot(row=1, col=0, title="Live FFT (dBFS)")
        self.curve_fft = self.plot_fft.plot(pen='c')
        self.peak_label = pg.TextItem(anchor=(0, 1), color='r')
        self.plot_fft.addItem(self.peak_label)
        self.plot_fft.setYRange(-100, 0)
        
        self.all_data = []
        self.fft_size = 2048
        self.fft_buffer = np.zeros(self.fft_size, dtype=np.int16)
        self.window = np.hanning(self.fft_size)
        
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        except Exception as e:
            print(f"❌ Connection Error: {e}"); sys.exit()

        self.start_time = time.time()
        self.recording_active = True
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.timer.start(20) 

    def update(self):
        if not self.recording_active: return
        
        elapsed = time.time() - self.start_time
        if self.ser.in_waiting >= 2:
            raw = self.ser.read(self.ser.in_waiting - (self.ser.in_waiting % 2))
            samples = np.frombuffer(raw, dtype=np.int16)
            self.all_data.append(samples)
            
            # Buffer Update
            self.fft_buffer = np.roll(self.fft_buffer, -len(samples))
            self.fft_buffer[-len(samples):] = samples if len(samples) <= self.fft_size else samples[-self.fft_size:]
            
            # Est Sample Rate for live plot (rough estimate)
            total_s = sum(len(c) for c in self.all_data)
            fs = total_s / elapsed if elapsed > 0 else 10000
            
            self.curve_time.setData(self.fft_buffer)
            
            windowed = self.fft_buffer * self.window
            mag = np.abs(np.fft.rfft(windowed))
            ref = (32768 * self.fft_size) / 2
            db_vals = 20 * np.log10(np.maximum(mag, 1e-10) / ref)
            
            freqs = np.fft.rfftfreq(self.fft_size, 1.0 / fs)
            self.curve_fft.setData(freqs, db_vals)
            
            peak_idx = np.argmax(mag[1:]) + 1
            self.peak_label.setText(f"Peak: {freqs[peak_idx]:.0f} Hz")
            self.peak_label.setPos(freqs[peak_idx], db_vals[peak_idx])

        if elapsed >= RECORD_DURATION:
            self.recording_active = False
            self.timer.stop(); self.ser.close()
            self.finish_and_save()

    def finish_and_save(self):
        raw_final = np.concatenate(self.all_data)
        
        # Calculate EXACT sample rate based on total data gathered
        fs = int(len(raw_final) / RECORD_DURATION)
        nyquist = fs / 2.0
        
        print(f"\n✅ Recording Complete. Exact Sample Rate: {fs} Hz")
        print("⚙️ Processing Bandpass Filter (20Hz - 20kHz)...")

        # --- High-Pass (20Hz) & Low-Pass (20kHz) Filter ---
        low_cutoff = 20.0
        high_cutoff = 20000.0
        
        # Safety check: if sample rate is low, limit the high cutoff to just below Nyquist
        if high_cutoff >= nyquist:
            high_cutoff = nyquist - 1.0
            
        # Design a 4th-order Butterworth Bandpass filter
        b, a = signal.butter(4, [low_cutoff / nyquist, high_cutoff / nyquist], btype='band')
        
        # filtfilt applies the filter forward and backward, resulting in ZERO phase distortion
        filt_final = signal.filtfilt(b, a, raw_final)

        # Save both waves
        for name, data in [('audio_raw.wav', raw_final), ('audio_bandpass.wav', filt_final)]:
            # Normalize to 16-bit PCM safely
            norm = ((data.astype(np.float32) / np.max(np.abs(data))) * 32767 * 0.95).astype(np.int16)
            with wave.open(name, 'w') as wf:
                wf.setnchannels(1)
                wf.setsampwidth(2)
                wf.setframerate(fs)
                wf.writeframes(norm.tobytes())

        print("\n--- Final Analysis ---")
        analyze_wav('audio_raw.wav', 'wav_analysis_raw.png')
        analyze_wav('audio_bandpass.wav', 'wav_analysis_bandpass.png')
        print("✅ Process Complete.")

    def run(self):
        sys.exit(self.app.exec_())

if __name__ == '__main__':
    AudioRecorder().run()
