import os
import time
import pandas as pd
import numpy as np
from scipy.signal import butter, filtfilt
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import paho.mqtt.client as mqtt

WATCH_FOLDER = "./data"

# Callback function
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code== 0:
        print("Connected to moqsuitto broker successfully!")
    else:
        print(f"Connection failed with code {reason_code}")

def on_message(client, userdata, msg):
    print(f"Inference Results: {msg.payload.decode()}")

class XLSHandler(FileSystemEventHandler):
    def on_created(self, event):
        if event.src_path.endswith(".xls") and not event.is_directory:
            print(f"New .xls file detected: {event.src_path}")
            try:

                # Load xls file (actually a tab delimiter file)
                df = pd.read_csv(event.src_path, sep='\t')  # shape ~ (80000, number_of_leads)

                # Drop the first and last columns (assuming they're named 'Time' and 'Spare')
                df = df.iloc[:, 1:-1]

                # Ensure column names are standard
                df.columns = [col.strip() for col in df.columns]

                # Extract the necessary leads
                LA = df['LA']
                LL = df['LL']
                RA = df['RA']
                V1 = df['V1']
                V2 = df['V2']
                V3 = df['V3']
                V4 = df['V4']
                V5 = df['V5']
                V6 = df['V6']

                L1 = LA-RA
                L2 = LL-RA
                L3 = LL-LA

                # Calculate augmented leads
                aVR = -0.5 * (L1 + L2)
                aVL = 0.5 * (L1 - L3)
                aVF = 0.5 * (L2 + L3)

                V1_prime=V1-0.333*(LA+RA+LL)
                V2_prime=V2-0.333*(LA+RA+LL)
                V3_prime=V3-0.333*(LA+RA+LL)
                V4_prime=V4-0.333*(LA+RA+LL)
                V5_prime=V5-0.333*(LA+RA+LL)
                V6_prime=V6-0.333*(LA+RA+LL)

                # Create a DataFrame for augmented leads
                lead_df = pd.DataFrame({'Lead I': L1, 'Lead II': L2, 'Lead III': L3, 'aVR': aVR, 'aVL': aVL, 'aVF': aVF,
                                        'V1': V1, 'V2': V2, 'V3': V3, 'V4': V4, 'V5': V5, 'V6': V6})


                # Define filter function
                def butter_lowpass_filter(data, cutoff, fs, order=5):
                    nyquist = 0.5 * fs
                    normal_cutoff = cutoff / nyquist
                    # Design Butterworth filter
                    b, a = butter(order, normal_cutoff, btype='low', analog=False)
                    # Apply filter
                    y = filtfilt(b, a, data, axis=0)
                    return y

                # Original sampling rate
                fs_original = 2000  # Hz

                # Target sampling rate
                fs_target = 500  # Hz
                downsample_factor = int(fs_original / fs_target)

                # Apply low-pass filter
                cutoff_freq = 200  # Hz (good for ECG)
                filtered_data = butter_lowpass_filter(lead_df.values, cutoff=cutoff_freq, fs=fs_original, order=5)

                # Downsample: take every 32nd sample
                downsampled_data = filtered_data[::downsample_factor]

                # Convert from Volts to millivolts
                downsampled_data_mV = downsampled_data * 1000.0  # Now in mV

                # Save or use the downsampled data
                data = pd.DataFrame(downsampled_data_mV, columns=lead_df.columns)
                data = data.T
                # Convert the entire DataFrame to a list of dicts (records)

                records = data.to_dict(orient='records')

                # You can also include metadata like filename if needed
                message = {
                    "filename": os.path.basename(event.src_path),
                    "records": records
                }

                # Convert to JSON string
                import json
                json_payload = json.dumps(message)

                # Publish to MQTT
                client.publish("ecg/data", json_payload)
                print(f"Published {len(records)} rows from {event.src_path}")

            except Exception as e:
                print(f"Error reading {event.src_path}: {e}")

# Define the MQTT broker settings and create client

client = mqtt.Client(client_id="ECG Data Client", userdata=None, protocol=mqtt.MQTTv5)
client.username_pw_set("", "")
client.on_connect = on_connect
client.on_message = on_message
client.connect("", 1883)


# Subscribe to a topic (optional)
client.subscribe("ecg/inference")

# Start the client loop
client.loop_start()

# Start watching the folder
event_handler = XLSHandler()
observer = Observer()
observer.schedule(event_handler, path=WATCH_FOLDER, recursive=False)
observer.start()

print(f"Watching folder: {WATCH_FOLDER} for new .xls files")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    observer.stop()
    client.loop_stop()
    print("Stopped monitoring.")

observer.join()