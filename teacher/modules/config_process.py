import json
import argparse

def read_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--conf", type=str, default="./config.json")
    parser.add_argument("--inf", type=str, default="./input.bin")
    parser.add_argument("--outf", type=str, default="./output.bin")    
    parser.add_argument("--ind", type=str, default="./input_dir")
    parser.add_argument("--outd", type=str, default="./output_dir")    
    args = parser.parse_args()
    return args

def read_config():
    args = read_parser()
    with open(args.conf) as json_file:
        config = json.load(json_file)
    return config

if __name__ == "__main__":
    conf = read_config()
    print(conf)
