import os
import json
import glob
import argparse

def file_split(input_file, output_path, split_size=32000):
    os.makedirs(output_path, exist_ok=True)
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    ext = os.path.splitext(input_file)[1]
    with open(input_file, "rb") as fin:
        idx = 0
        while True:
            chunk = fin.read(split_size)
            if not chunk:
                break
            output_file = os.path.join(output_path, f"{base_name}_{idx:04d}{ext}")
            with open(output_file, "wb") as fout:
                fout.write(chunk)
            idx += 1
    return idx


def read_input_filenames(input_path):
    input_files = glob.glob(os.path.join(input_path, '*.*'))
    return sorted(input_files)


def sample_process(input_path, output_path):
    input_files = read_input_filenames(input_path)
    for input_file in input_files:
        base_name = os.path.splitext(os.path.basename(input_file))[0]
        output_dir = os.path.join(output_path, base_name)
        file_split(input_file, output_dir)


if __name__ == "__main__":
    input_path = "../dataset/raw"
    output_path = "../dataset/sample"
    sample_process(input_path, output_path)
