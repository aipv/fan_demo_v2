import os
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score,confusion_matrix,classification_report

def load_dsp32_group(dsp32_path, group, n_mfcc=49, n_coef=40):
    dsp32_file = os.path.join(dsp32_path, group + ".bin")
    dsp32_data = np.fromfile(dsp32_file, dtype=np.float32)
    count = dsp32_data.size // n_mfcc // n_coef
    result = dsp32_data.reshape((count, n_mfcc, n_coef))
    result = result.mean(axis=1)
    return result

def load_dsp32_groups(dsp32_path, groups):
    result = np.concatenate(
        [load_dsp32_group(dsp32_path, group) for group in groups],
        axis=0
    )
    return result

def load_train_dataset(dsp32_path, pos_group, neg_group):
    pos_data = load_dsp32_groups(dsp32_path, pos_group)
    neg_data = load_dsp32_groups(dsp32_path, neg_group)
    x = np.concatenate([pos_data, neg_data], axis=0)
    y = np.concatenate([
        np.zeros(len(pos_data), dtype=np.int32),
        np.ones(len(neg_data), dtype=np.int32)
    ])
    x = x.reshape(len(x),-1)
    return x, y

def split_train_dataset(x, y):
    x_train, x_test, y_train, y_test = train_test_split(x,y,
        test_size=0.2, random_state=42, stratify=y)
    return x_train, x_test, y_train, y_test

def model_train_dataset(x, y):
    model=LogisticRegression(max_iter=5000, n_jobs=-1)
    model.fit(x, y)
    return model

def model_validate_dataset(model, x, y):
    y_pred = model.predict(x)
    prob = model.predict_proba(x)
    print("\nAccuracy =",accuracy_score(y, y_pred))
    print("\nConfusion Matrix")
    print(confusion_matrix(y, y_pred))
    print("\nClassification Report")
    print(classification_report(y, y_pred))
    print("\nNormal Prob Mean =",prob[y==0][:,0].mean())
    print("Fault  Prob Mean =",prob[y==1][:,1].mean())

def model_train(dataset, pos_group, neg_group):
    x, y = load_train_dataset(dataset, pos_group, neg_group)
    x_train, x_test, y_train, y_test = split_train_dataset(x, y)
    model = model_train_dataset(x_train, y_train)
    model_validate_dataset(model, x_test, y_test)
    return model

def eval_group(model, dsp32_dir, group):
    data=load_dsp32_group(dsp32_dir,group)
    x=data.reshape(len(data),-1)
    p=model.predict_proba(x)[:,1]
    print(group, "mean=",p.mean(), "std=",p.std(), "max=",p.max())

def eval_groups(model, dsp32_dir, groups):
    for group in groups:
        eval_group(model, dsp32_dir, group)

if __name__=="__main__":
    path="../dataset/dsp32"
    pos_group = ['normal']
    neg_group = ['abnormal', 'd1', 'd2']
    val_group = ['normal', 'abnormal', 'd1', 'd2', 'd3', 'd4']
    model = model_train(path, pos_group, neg_group)
    eval_groups(model, path, val_group)
