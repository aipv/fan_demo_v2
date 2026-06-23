def export_py_model(model, output_file):
    coef = model.coef_[0]
    bias = model.intercept_[0]
    with open(output_file, "w") as f:
        f.write("# Auto Generated\n\n")
        f.write("import numpy as np\n\n")
        f.write(f"BIAS = {bias:.9f}\n\n")
        f.write("WEIGHT = [\n")
        for row in range(5):
            start = row * 8
            end = start + 8
            values = [f"{coef[i]: .9f}" for i in range(start, end)]
            f.write("    ")
            f.write(", ".join(values))
            f.write(",\n")
        f.write("]\n\n")
        f.write("""
def predict_score(feature):
    feature = np.asarray(feature, dtype=np.float32)
    return np.dot(feature, WEIGHT) + BIAS

def predict_prob(feature):
    score = predict_score(feature)
    return 1.0 / (1.0 + np.exp(-score))

def predict_class(feature, threshold=0.5):
    prob = predict_prob(feature)
    return 1 if prob >= threshold else 0

def predict_batch(features):
    features = np.asarray(features, dtype=np.float32)
    scores = np.dot(features, WEIGHT) + BIAS
    probs = 1.0 / (1.0 + np.exp(-scores))
    return probs
""")

def export_h_model(model, output_file):
    coef = model.coef_[0]
    bias = model.intercept_[0]
    with open(output_file, "w") as f:
        f.write("#ifndef FAN_MODEL_H\n")
        f.write("#define FAN_MODEL_H\n\n")
        f.write(f"static const float g_bias = {bias:.9f}f;\n\n")
        f.write("static const float g_weight[40] = {\n")
        for row in range(5):
            start = row * 8
            end = start + 8
            values = [f"{coef[i]:.9f}f" for i in range(start, end)]
            f.write("    " + ", ".join(values) + ",\n")
        f.write("};\n\n")
        f.write("float predict_score(const float* feature);\n")
        f.write("float predict_prob(const float* feature);\n")
        f.write("int predict_class(const float* feature, float threshold);\n")
        f.write("void predict_batch(const float* features, float* probs, int count);\n\n")
        f.write("#endif\n")


def export_c_model(model, output_file):
    coef = model.coef_[0]
    bias = model.intercept_[0]
    with open(output_file, "w") as f:
        f.write("#include \"model.h\"\n")
        f.write("#include <math.h>\n\n")
        f.write("float predict_score(const float* feature) {\n")
        f.write("    float score = g_bias;\n")
        f.write("    for (int i = 0; i < 40; i++) {\n")
        f.write("        score += feature[i] * g_weight[i];\n")
        f.write("    }\n")
        f.write("    return score;\n")
        f.write("}\n\n")
        f.write("float predict_prob(const float* feature) {\n")
        f.write("    float score = predict_score(feature);\n")
        f.write("    return 1.0f / (1.0f + expf(-score));\n")
        f.write("}\n\n")
        f.write("int predict_class(const float* feature, float threshold) {\n")
        f.write("    return predict_prob(feature) >= threshold ? 1 : 0;\n")
        f.write("}\n\n")
        f.write("void predict_batch(const float* features, float* probs, int count) {\n")
        f.write("    for (int i = 0; i < count; i++) {\n")
        f.write("        probs[i] = predict_prob(&features[i * 40]);\n")
        f.write("    }\n")
        f.write("}\n")

