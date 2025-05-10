import os
import numpy as np
import torch
import paho.mqtt.client as mqtt
import json
import pandas as pd
from fairseq_signals.models import build_model_from_checkpoint

root=os.getcwd()

model_finetuned = build_model_from_checkpoint(
    checkpoint_path=os.path.join(root, 'ckpts/physionet_finetuned.pt')
)

model_finetuned = model_finetuned.to("cpu")

labels = ['AF', 'AFL', 'BBB', 'Brady', 'CLBBB|LBBB', 'CRBBB|RBBB', 'IAVB', 'IRBBB', 'LAD', 'LAnFB', 'LPR', 'LQRSV', 'LQT',
    'NSIVCB', 'NSR', 'PAC|SVPB', 'PR', 'PRWP', 'PVC|VPB', 'QAb', 'RAD', 'SA', 'SB', 'STach', 'TAb', 'TInv']

def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected to broker")
    client.subscribe("ecg/data")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)

        records = data.get("records", [])
        df = pd.DataFrame(records)

        feats = df.values
        feats = torch.from_numpy(feats).float()
        feats = feats.unsqueeze(0)

        print(f"Received DataFrame shape: {feats.shape}")

        inputs = {"net_input": {"source": feats}}
        with torch.no_grad():
            net_output = model_finetuned(**inputs["net_input"])

        logits = net_output['out']
        predictions = torch.sigmoid(logits)

        print("prediction confidence", predictions)

        mask = predictions.squeeze(0) > 0.9
        print("prediction truth table: ", mask)

        print(len(labels), mask.shape)
        print("Mask as list:", mask.tolist())

        if predictions.any():
            filtered_prediction = [label for label, keep in zip(labels, mask.tolist()) if keep]
            print("Filtered labels:", filtered_prediction)
            message = ','.join(filtered_prediction)
            print(message)
            client.publish("ecg/inference", message)
        else:
            client.publish("ecg/inference", "Normal")

    except Exception as e:
        print(f"Error processing message: {e}")

client = mqtt.Client(
    client_id="ECG Inference Client",
    protocol=mqtt.MQTTv5
)
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set("", "")
client.connect("localhost", 1883)
client.loop_forever()
