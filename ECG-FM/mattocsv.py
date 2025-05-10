import scipy.io
import torch
import pandas as pd
mat = scipy.io.loadmat('/home/ubuntu/ECG-FM/data/code_15/segmented/code_15_1238362_0.mat') 
x=mat["feats"]
tensor = torch.from_numpy(x).float()

df=pd.DataFrame(x, index=['I', 'II', 'III', 'aVR', 'aVL', 'aVF', 'V1', 'V2', 'V3', 'V4', 'V5', 'V6'])

df_transposed = df.T
df_transposed.to_csv('/home/ubuntu/ECG-FM/datatest.csv', index=False)

print(tensor)
print(tensor.dtype)
