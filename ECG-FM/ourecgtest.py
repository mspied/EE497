import os
import pymatreader
import numpy as np
import torch
import pandas as pd
from fairseq_signals.models import build_model_from_checkpoint

root=os.getcwd()

model_finetuned = build_model_from_checkpoint(
    checkpoint_path=os.path.join(root, 'ckpts/physionet_finetuned.pt')
)

model_finetuned = model_finetuned.to("cpu")
model_finetuned.eval()

df = pd.read_csv('/home/ubuntu/ECG-FM/ourdata/sample5_12leads_zeroed.csv')
df = df.T
feats_csv = df.values
print("feats csv numpy array data", feats_csv)
print("\n")
feats_csv = torch.from_numpy(feats_csv).float()

feats_csv = feats_csv.unsqueeze(0)
print(f"Processed input shape: {feats_csv.shape}")

inputs_csv = {"net_input": {"source": feats_csv}}
with torch.no_grad():
        net_output_csv = model_finetuned(**inputs_csv["net_input"])
logits_csv = net_output_csv['out']
predictions_csv = torch.sigmoid(logits_csv)
print("csv sigmoid predictions", predictions_csv)

predictions_csv = predictions_csv > 0.8
print("thresholded predictions", predictions_csv)
