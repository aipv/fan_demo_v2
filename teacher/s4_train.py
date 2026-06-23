import os
from modules.config_process import read_config
from modules.train_process import model_train, eval_groups
from modules.model_process import export_py_model, export_h_model, export_c_model

def s4_dataset_train(conf):
    dsp32_path = conf['dsp32']['output']
    val_group = conf['dsp32']['groups']
    pos_group = conf['train']['pos_group']
    neg_group = conf['train']['neg_group']
    model_file_py = conf['train']['model_py']
    model_file_h = conf['train']['model_h']
    model_file_c = conf['train']['model_c']
    print(model_file_py, model_file_h, model_file_c)
    model = model_train(dsp32_path, pos_group, neg_group)
    export_py_model(model, model_file_py)
    export_h_model(model, model_file_h)
    export_c_model(model, model_file_c)
    eval_groups(model, dsp32_path, val_group)

if __name__ == "__main__":
    conf = read_config()
    s4_dataset_train(conf)
