from modules.config_process import read_config
from modules.sample_process import sample_process

if __name__ == "__main__":
    conf = read_config()['sample']
    sample_process(conf['input'], conf['output'])
