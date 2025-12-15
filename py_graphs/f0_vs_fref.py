#!/usr/bin/env python3
import argparse
import numpy as np
import matplotlib.pyplot as plt

def read_f0_txt(path: str) -> np.ndarray:
    vals = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            try:
                vals.append(float(s))
            except ValueError:
                raise ValueError(f"Línea no numérica en {path!r}: {line!r}")
    return np.asarray(vals, dtype=float)

def read_wav(path: str):
    # Preferimos soundfile si está, y si no, caemos a scipy.
    try:
        import soundfile as sf
        x, sr = sf.read(path, always_2d=False)
    except Exception:
        from scipy.io import wavfile
        sr, x = wavfile.read(path)
        # normaliza enteros a [-1, 1]
        if np.issubdtype(x.dtype, np.integer):
            x = x.astype(np.float32) / np.iinfo(x.dtype).max

    # Si es estéreo/multicanal, lo pasamos a mono promediando
    if x.ndim > 1:
        x = x.mean(axis=1)

    return x.astype(np.float32), int(sr)

def main():
    ap = argparse.ArgumentParser(description="Plot waveform + f0/f0ref points over time.")
    ap.add_argument("--wav", required=True, help="Ruta al archivo .wav")
    ap.add_argument("--f0", required=True, help="Ruta al archivo .f0 (texto, 1 valor por línea)")
    ap.add_argument("--f0ref", required=True, help="Ruta al archivo .f0ref (texto, 1 valor por línea)")
    ap.add_argument("--win-ms", type=float, default=30.0, help="Tamaño de ventana en ms (default: 30)")
    ap.add_argument("--shift-ms", type=float, default=15.0, help="Shift entre ventanas en ms (default: 15)")
    ap.add_argument("--out", default=None, help="Si se indica, guarda la figura en este path (png/pdf/etc).")
    ap.add_argument("--dpi", type=int, default=200, help="DPI al guardar (default: 200)")
    args = ap.parse_args()

    x, sr = read_wav(args.wav)
    f0 = read_f0_txt(args.f0)
    f0ref = read_f0_txt(args.f0ref)

    # Eje temporal del wav (segundos)
    t_wav = np.arange(len(x)) / sr
    dur = t_wav[-1] if len(t_wav) else 0.0

    # Tiempos de frames (centro de ventana), en segundos
    win = args.win_ms / 1000.0
    shift = args.shift_ms / 1000.0

    n_frames = min(len(f0), len(f0ref))
    if len(f0) != len(f0ref):
        print(f"[warn] f0 tiene {len(f0)} frames y f0ref tiene {len(f0ref)}. Uso min={n_frames}.")

    idx = np.arange(n_frames)
    t_frames = idx * shift + 0.5 * win  # centro de ventana

    # Máscaras: solo puntos con valor != 0
    m_f0 = f0[:n_frames] != 0
    m_ref = f0ref[:n_frames] != 0

    # (Opcional) si algunos frames caen fuera de la duración del wav, los filtramos
    in_range = (t_frames >= 0) & (t_frames <= dur + 1e-9)
    m_f0 &= in_range
    m_ref &= in_range

    # Figura
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6), sharex=True)

    # Subplot 1: waveform
    ax1.plot(t_wav, x, linewidth=0.8)
    ax1.set_ylabel("Amplitud")
    ax1.set_title("Waveform")

    # Subplot 2: puntos f0 y f0ref
    ax2.scatter(t_frames[m_f0], f0[:n_frames][m_f0], s=12, marker="o", label="f0", color="red")
    ax2.scatter(t_frames[m_ref], f0ref[:n_frames][m_ref], s=12, marker="o", label="f0ref", color="green")
    ax2.set_xlabel("Tiempo (s)")
    ax2.set_ylabel("Frecuencia (Hz)")
    ax2.set_title("Pitch (f0 vs f0ref)")
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    # Límites X: del wav (0..dur)
    ax2.set_xlim(0, max(dur, (t_frames[in_range].max() if in_range.any() else 0.0)))
    ax2.set_ylim(0, 350)

    plt.tight_layout()

    if args.out:
        fig.savefig(args.out, dpi=args.dpi, bbox_inches="tight")
        print(f"Guardado: {args.out}")
    else:
        plt.show()

if __name__ == "__main__":
    main()
