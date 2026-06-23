import os
import sys
sys.path.insert(0, '../dataset/model')
import numpy as np
from model import predict_batch
from modules.train_process import load_dsp32_group, load_train_dataset, split_train_dataset
from sklearn.linear_model import LogisticRegression


def load_sklearn_model():
    """Retrain and return sklearn model"""
    dsp32_path = "../dataset/dsp32"
    pos_group = ["normal"]
    neg_group = ["abnormal", "d1", "d2"]

    x, y = load_train_dataset(dsp32_path, pos_group, neg_group)
    x_train, x_test, y_train, y_test = split_train_dataset(x, y)
    model = LogisticRegression(max_iter=5000)
    model.fit(x_train, y_train)
    return model


def compare_and_save(dsp32_dir, groups, output_dir):
    os.makedirs(output_dir, exist_ok=True)

    sklearn_model = load_sklearn_model()

    print(f"{'Group':<10} {'Max Diff':<15} {'Mean Diff':<15} {'SKLearn Mean':<15} {'Model Mean':<15}")
    print("-" * 70)

    for group in groups:
        data = load_dsp32_group(dsp32_dir, group)
        custom_probs = predict_batch(data)
        sklearn_probs = sklearn_model.predict_proba(data)[:, 1]

        max_diff = np.abs(custom_probs - sklearn_probs).max()
        mean_diff = np.abs(custom_probs - sklearn_probs).mean()

        print(f"{group:<10} {max_diff:<15.10f} {mean_diff:<15.10f} {sklearn_probs.mean():<15.9f} {custom_probs.mean():<15.9f}")

        custom_probs.astype(np.float32).tofile(os.path.join(output_dir, f"{group}.bin"))

    print("-" * 70)
    print("All predictions written to", output_dir)


if __name__ == "__main__":
    dsp32_dir = "../dataset/dsp32"
    groups = ["normal", "abnormal", "d1", "d2", "d3", "d4"]
    output_dir = "../dataset/predict"
    compare_and_save(dsp32_dir, groups, output_dir)
