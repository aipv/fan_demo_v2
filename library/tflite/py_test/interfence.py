import os
import numpy as np
#import tensorflow as tf
from tflite_runtime.interpreter import Interpreter

MFCC_H = 98
MFCC_W = 13
MFCC_SIZE = MFCC_H * MFCC_W

TFLITE_MODEL_PATH = "../../../dataset/d5_training/model_int8.tflite"
TEST_POS_PATH     = "../../../dataset/d4_mfcc_feature/dsp08/test_pos"
TEST_NEG_PATH     = "../../../dataset/d4_mfcc_feature/dsp08/test_neg"

THRESHOLD = 21


def load_model():
    interpreter = Interpreter(
        model_path=TFLITE_MODEL_PATH
    )

    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    print("Input :", input_details)
    print("Output:", output_details)

    return interpreter


def inference(interpreter, mfcc):
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    input_data = mfcc.reshape(1, 98, 13, 1)

    interpreter.set_tensor(
        input_details[0]['index'],
        input_data
    )

    interpreter.invoke()

    output = interpreter.get_tensor(
        output_details[0]['index']
    )

    score = int(output.flatten()[0])

    return score


def read_mfcc(filename):
    mfcc = np.fromfile(
        filename,
        dtype=np.int8
    )

    if mfcc.size != MFCC_SIZE:
        raise RuntimeError(
            f"Bad MFCC size: {filename}"
        )

    return mfcc


def process_folder(
        interpreter,
        folder,
        is_positive):

    tp = tn = fp = fn = 0

    min_score = 127
    max_score = -128
    score_sum = 0
    score_count = 0

    files = sorted(os.listdir(folder))

    for f in files:

        path = os.path.join(folder, f)

        mfcc = read_mfcc(path)

        score = inference(
            interpreter,
            mfcc
        )

        print(f"{path} : {score}")

        min_score = min(min_score, score)
        max_score = max(max_score, score)

        score_sum += score
        score_count += 1

        pred = score > THRESHOLD

        if is_positive:
            if pred:
                tp += 1
            else:
                fn += 1
        else:
            if pred:
                fp += 1
            else:
                tn += 1

    mean_score = score_sum / score_count

    return tp, tn, fp, fn, \
           min_score, max_score, mean_score


def main():

    interpreter = load_model()

    pos = process_folder(
        interpreter,
        TEST_POS_PATH,
        True
    )

    neg = process_folder(
        interpreter,
        TEST_NEG_PATH,
        False
    )

    tp = pos[0]
    fn = pos[3]

    tn = neg[1]
    fp = neg[2]

    total = tp + tn + fp + fn

    acc = (tp + tn) / total

    print()
    print("=" * 60)
    print("RESULT")
    print("=" * 60)

    print(f"TP = {tp}")
    print(f"FN = {fn}")
    print(f"TN = {tn}")
    print(f"FP = {fp}")

    print()
    print(f"Accuracy = {acc:.6f}")

    print()
    print("=" * 60)
    print("SCORES")
    print("=" * 60)

    min_score = min(pos[4], neg[4])
    max_score = max(pos[5], neg[5])

    total_mean = (
        pos[6] * (tp + fn) +
        neg[6] * (tn + fp)
    ) / total

    print(f"min  = {min_score}")
    print(f"max  = {max_score}")
    print(f"mean = {total_mean:.4f}")


if __name__ == "__main__":
    main()