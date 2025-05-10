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

# Load the .mat file
file_path = '/home/ubuntu/ECG-FM/data/code_15/segmented/code_15_1238362_0.mat'
mat_data = pymatreader.read_mat(file_path)

# Print the type and shape of 'feats' if it exists
if 'feats' in mat_data:
    feats = mat_data['feats']
    print(type(feats))
    if isinstance(feats, list):
        print(f"List length: {len(feats)}")
    elif isinstance(feats, dict):
        print(f"Dict keys: {feats.keys()}")
    elif isinstance(feats, (np.ndarray, torch.Tensor)):
        print(f"Array shape: {feats.shape}")

print("feats mat numpy array data", feats)
print("\n")
# Convert to PyTorch tensor
if isinstance(feats, list):
    feats = torch.tensor(feats, dtype=torch.float32)
elif isinstance(feats, np.ndarray):
    feats = torch.from_numpy(feats).float()

# Add batch dimension if necessary
feats = feats.unsqueeze(0)  # Shape: [1, num_features] (adjust if needed)

feats = feats.to("cpu")

print(f"Processed input shape: {feats.shape}")
print("\n")

model_finetuned.eval()
inputs = {"net_input": {"source": feats}}
with torch.no_grad():
	net_output = model_finetuned(**inputs["net_input"])

print("raw output", net_output)
logits = net_output['out']
predictions = torch.sigmoid(logits)
print("sigmoid predictions", predictions)

predictions = predictions > 0.8
print("thresholded predictions", predictions)


df = pd.read_csv('/home/ubuntu/ECG-FM/ourdata/sample3_12leads_zeroed.csv')
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
