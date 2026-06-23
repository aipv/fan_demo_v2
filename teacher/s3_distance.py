import os
from modules.config_process import read_config
from modules.signal_process import load_dsp32_data, load_dsp32_distance


def s3_dsp_distance(conf):
    dsp32_path = conf['output']
    groups = conf['groups']
    result = load_dsp32_data(dsp32_path, groups)
    normal_mean = result['normal'].mean(axis=0)
    for group in groups:
        load_dsp32_distance(result[group], group, normal_mean)

if __name__ == "__main__":
    conf = read_config()['dsp32']
    s3_dsp_distance(conf)
