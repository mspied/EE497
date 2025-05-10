import os
import pandas as pd
import torch

from fairseq_signals.utils.store import MemmapReader

root = os.getcwd()
fairseq_signals_root = '/home/ubuntu/fairseq-signals'
fairseq_signals_root = fairseq_signals_root.rstrip('/')

assert os.path.isfile(os.path.join(root, 'ckpts/physionet_finetuned.pt'))
assert os.path.isfile(os.path.join(root, 'ckpts/physionet_finetuned.yaml'))

segmented_split = pd.read_csv(
    os.path.join(root, 'data/code_15/segmented_split_incomplete.csv'),
    index_col='idx',
)
segmented_split['path'] = (root + '/data/code_15/segmented/') + segmented_split['path']
segmented_split.to_csv(os.path.join(root, 'data/code_15/segmented_split.csv'))

assert os.path.isfile(os.path.join(root, 'data/code_15/segmented_split.csv'))

print(f"""cd {fairseq_signals_root}/scripts/preprocess
python manifests.py \\
    --split_file_paths "{root}/data/code_15/segmented_split.csv" \\
    --save_dir "{root}/data/manifests/code_15_subset10/"
""")

assert os.path.isfile(os.path.join(root, 'data/manifests/code_15_subset10/test.tsv'))

print(f"""fairseq-hydra-inference \\
    task.data="{root}/data/manifests/code_15_subset10/" \\
    common_eval.path="{root}/ckpts/physionet_finetuned.pt" \\
    common_eval.results_path="{root}/outputs" \\
    model.num_labels=26 \\
    dataset.valid_subset="test" \\
    dataset.batch_size=10 \\
    dataset.num_workers=3 \\
    dataset.disable_validation=false \\
    distributed_training.distributed_world_size=1 \\
    distributed_training.find_unused_parameters=True \\
    --config-dir "{root}/ckpts" \\
    --config-name physionet_finetuned
""")
