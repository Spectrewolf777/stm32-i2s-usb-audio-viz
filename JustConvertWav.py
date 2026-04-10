import serial
import wave
import sys

# --- CONFIGURATION ---
SERIAL_PORT = '/dev/ttyACM0'        # Change to your port (e.g., '/dev/ttyACM0' on Linux)
BAUD_RATE = 115200          # For CDC, this is often ignored but kept for compatibility
OUTPUT_FILENAME = "recording.wav"

# Audio Settings (Must match your STM32 configuration)
CHANNELS = 2               # 1 for Mono, 2 for Stereo
SAMPLE_RATE = 44100         # e.g., 16000, 44100, 48000
SAMPLE_WIDTH = 2            # 2 bytes for 16-bit audio
CHUNK_SIZE = 4096           # Matches your AUDIO_BUFFER_SIZE

def record_audio():
    try:
        # Initialize Serial
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT}. Recording... Press Ctrl+C to stop.")

        # Initialize WAV file
        with wave.open(OUTPUT_FILENAME, 'wb') as wav_file:
            wav_file.setnchannels(CHANNELS)
            wav_file.setsampwidth(SAMPLE_WIDTH)
            wav_file.setframerate(SAMPLE_RATE)

            while True:
                # Read raw bytes from serial
                data = ser.read(CHUNK_SIZE)
                
                if data:
                    wav_file.writeframes(data)
                    
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except KeyboardInterrupt:
        print("\nRecording stopped and saved.")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    record_audio()