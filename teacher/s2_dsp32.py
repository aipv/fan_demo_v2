import os
from modules.config_process import read_config
from modules.signal_process import dsp32_mfcc_feature_signal_process


def s2_dsp32_process(conf):
    input_dir = conf['input']
    output_dir = conf['output']
    groups = conf['groups']
    dsp32_mfcc_feature_signal_process(input_dir, output_dir, groups)


if __name__ == "__main__":
    conf = read_config()['dsp32']
    s2_dsp32_process(conf)
