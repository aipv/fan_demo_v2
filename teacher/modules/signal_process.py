import os
import glob
import numpy as np
from scipy.fft import fft as scipy_fft
from scipy.fft import dct as scipy_dct

def _hz_to_mel(hz):
    return 2595.0 * np.log10(1.0 + hz / 700.0)

def _mel_to_hz(mel):
    return 700.0 * (10.0**(mel / 2595.0) - 1.0)

def _get_mel_filter_bank(fs=16000, n_fft=512, n_mel_filters=40):
    low_mel = _hz_to_mel(0)
    high_mel = _hz_to_mel(fs / 2)
    mel_points = np.linspace(low_mel, high_mel, n_mel_filters + 2)
    hz_points = _mel_to_hz(mel_points)
    bin_points = np.floor((n_fft + 1) * hz_points / fs).astype(int)
    filter_bank = np.zeros((n_mel_filters, n_fft // 2 + 1))
    for m in range(1, n_mel_filters + 1):
        f_m_minus = bin_points[m - 1]
        f_m = bin_points[m]
        f_m_plus = bin_points[m + 1]
        for k in range(f_m_minus, f_m):
            if k >= 0 and k < filter_bank.shape[1]:
                filter_bank[m - 1, k] = (k - f_m_minus) / (f_m - f_m_minus)
        for k in range(f_m, f_m_plus):
            if k >= 0 and k < filter_bank.shape[1]:
                # 线性斜坡
                filter_bank[m - 1, k] = (f_m_plus - k) / (f_m_plus - f_m)                
    return filter_bank

def dsp32_get_mfcc_feature(pcm16, mel_basis, N_FFT=512):
    padded_array = np.pad(pcm16, pad_width=(0, 112), mode='constant', constant_values=0)
    float_samples = padded_array.astype(np.float32)
    window = np.hanning(N_FFT)
    windowed_samples = float_samples * window
    fft_result = scipy_fft(windowed_samples, N_FFT)
    fft_magnitude = np.abs(fft_result[:N_FFT // 2 + 1])
    power_spectrum = (fft_magnitude ** 2) / N_FFT
    mel_energies = np.dot(power_spectrum, mel_basis.T)
    log_mel_energies = np.log(mel_energies.clip(min=1e-12))
    #mfccs = scipy_dct(log_mel_energies, type=2, axis=-1, norm='ortho')
    #final_mfccs = mfccs[:n_coef]
    return log_mel_energies[0:40]

# 49 * 40
def dsp32_get_mfcc_feature_in_one_frame(pcm16, mel_basis, hop_size=320, bin_size=400, N_FFT=512):
    result = []
    if (pcm16.size < bin_size):
        return result
    n_mfcc = 1 + (pcm16.size - bin_size) // hop_size
    for i in range(n_mfcc):
        mel40 = dsp32_get_mfcc_feature(pcm16[hop_size * i:hop_size * i + bin_size], mel_basis, N_FFT)
        result.append(mel40)
    return result


def dsp32_mfcc_feature_one_file(input_file, output_file):
    mel_basis = _get_mel_filter_bank()
    pcm16 = np.fromfile(input_file, dtype=np.int16)
    result = dsp32_get_mfcc_feature_in_one_frame(pcm16, mel_basis)
    np.asarray(result, dtype=np.float32).tofile(output_file)


def dsp32_mfcc_feature_one_folder(input_path, output_path):
    os.makedirs(output_path, exist_ok=True)
    mel_basis = _get_mel_filter_bank()
    input_files = glob.glob(os.path.join(input_path, '*.pcm'))
    for input_file in sorted(input_files):
        base_name = os.path.basename(input_file).replace(".pcm", ".bin")
        output_file = os.path.join(output_path, base_name)
        pcm16 = np.fromfile(input_file, dtype=np.int16)
        ret = dsp32_get_mfcc_feature_in_one_frame(pcm16, mel_basis)
        dsp32 = np.asarray(ret, dtype=np.float32)
        dsp32.tofile(output_file)


def dsp32_mfcc_feature_folder_to_file(input_path, output_file):
    mel_basis = _get_mel_filter_bank()
    input_files = glob.glob(os.path.join(input_path, '*.pcm'))
    result = []
    for input_file in sorted(input_files):
        pcm16 = np.fromfile(input_file, dtype=np.int16)
        ret = dsp32_get_mfcc_feature_in_one_frame(pcm16, mel_basis)
        result.append(ret)
    dsp32 = np.asarray(result, dtype=np.float32)
    dsp32.tofile(output_file)


def dsp32_mfcc_feature_folder_process(input_path, output_path, output_file):
    os.makedirs(output_path, exist_ok=True)
    mel_basis = _get_mel_filter_bank()
    input_files = glob.glob(os.path.join(input_path, '*.pcm'))
    result = []
    for input_file in sorted(input_files):
        base_name = os.path.basename(input_file).replace(".pcm", ".bin")
        pcm16 = np.fromfile(input_file, dtype=np.int16)
        ret = dsp32_get_mfcc_feature_in_one_frame(pcm16, mel_basis)
        result.append(ret)
        dsp32 = np.asarray(ret, dtype=np.float32)
        dsp32.tofile(os.path.join(output_path, base_name))
    dsp32 = np.asarray(result, dtype=np.float32)
    dsp32.tofile(output_file)
    print(dsp32.shape)


def dsp32_mfcc_feature_signal_process(input_dir, output_dir, groups):
    for group in groups:
        input_path = os.path.join(input_dir, group)
        output_path = os.path.join(output_dir, group)
        output_file = os.path.join(output_dir, group + ".bin")
        dsp32_mfcc_feature_folder_process(input_path, output_path, output_file)


def load_dsp32_data(dsp32_dir, groups, n_mfcc=49, n_coef=40):
    result = {}
    for group in groups:
        dsp32_file = os.path.join(dsp32_dir, group + ".bin")
        dsp32_data = np.fromfile(dsp32_file, dtype=np.float32)
        count = dsp32_data.size // n_mfcc // n_coef
        np_data = dsp32_data.reshape((count, n_mfcc, n_coef))
        result[group] = np_data
    return result


def load_dsp32_distance(dsp32, group, normal_center):
    scores = []
    for sample in dsp32:
        dist = np.linalg.norm(sample - normal_center)
        scores.append(dist)
    scores = np.asarray(scores)
    print("================", group, "================")
    print("    mean:", scores.mean())
    print("    std :", scores.std())
    print("    max :", scores.max())
    print("    P95 :", np.percentile(scores, 95))
    print("    P90 :", np.percentile(scores, 90))
    return scores


if __name__ == "__main__":
    input_path = "../dataset/sample"
    output_path = "../dataset/dsp32"
    groups = ["d4"]
    dsp32_mfcc_feature_signal_process(input_path, output_path, groups)

