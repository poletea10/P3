#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt


# ========= CAMBIA ESTO =========
OUTPUT_IMAGE_PATH = "img/comparacion_metricas.png"
# ===============================


def read_f0_file(path: str) -> np.ndarray:
    vals = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            vals.append(float(s))
    return np.asarray(vals, dtype=float)


def read_metrics_txt(path: str) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Lee un .txt con 3 columnas separadas por espacios (cantidad variable).
    Ejemplo por línea:
        -42.123    0.83      0.71
    """
    col1, col2, col3 = [], [], []
    with open(path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, start=1):
            s = line.strip()
            if not s:
                continue

            # split() ya ignora espacios múltiples / tabs (lo que tú describes)
            parts = s.split()
            if len(parts) < 3:
                raise ValueError(f"Línea {lineno} en {path} no tiene 3 columnas: {line!r}")

            col1.append(float(parts[0]))
            col2.append(float(parts[1]))
            col3.append(float(parts[2]))

    return (np.asarray(col1, dtype=float),
            np.asarray(col2, dtype=float),
            np.asarray(col3, dtype=float))


def main():
    import argparse

    ap = argparse.ArgumentParser(description="Plot pitch + power + autocorr metrics (4 subplots) per frame.")
    ap.add_argument("--f0", required=True, help="Archivo .f0 (1 valor por línea)")
    ap.add_argument("--txt", required=True, help="Archivo .txt (3 columnas por línea: power, r1/r0, rmax/r0)")
    args = ap.parse_args()

    f0 = read_f0_file(args.f0)
    power_db, r1_norm, rmax_norm = read_metrics_txt(args.txt)

    # Alineamos longitudes (por si hay discrepancias)
    n = min(len(f0), len(power_db), len(r1_norm), len(rmax_norm))
    if n == 0:
        raise RuntimeError("No hay datos para graficar (archivos vacíos o mal formateados).")

    if not (len(f0) == len(power_db) == len(r1_norm) == len(rmax_norm)):
        print(f"[warn] Longitudes distintas (f0={len(f0)}, txt={len(power_db)}). Uso n={n} tramas.")

    f0 = f0[:n]
    power_db = power_db[:n]
    r1_norm = r1_norm[:n]
    rmax_norm = rmax_norm[:n]

    frames = np.arange(n)

    fig, axes = plt.subplots(4, 1, figsize=(16, 9), sharex=True)

    # (1) Pitch
    axes[0].plot(frames, f0, color="green", linewidth=2.0)
    axes[0].set_title("Contorno de pitch estimado por wavesurfer")
    axes[0].set_ylabel("Pitch [Hz]")
    axes[0].grid(True, alpha=0.35)

    # (2) Potencia
    axes[1].plot(frames, power_db, color="black", linewidth=2.0)
    axes[1].set_title("Nivel de potencia senyal")
    axes[1].set_ylabel("r[0] [dB]")
    axes[1].grid(True, alpha=0.35)

    # (3) r[1]/r[0]
    axes[2].plot(frames, r1_norm, color="blue", linewidth=2.0)
    axes[2].set_title("Relación r[1]/r[0]")
    axes[2].set_ylabel("r1norm")
    axes[2].grid(True, alpha=0.35)

    # (4) r[lag]/r[0] al segon màxim
    axes[3].plot(frames, rmax_norm, color="red", linewidth=2.0)
    axes[3].set_title("Relación pico secundario/potencia")
    axes[3].set_ylabel("rmaxnorm")
    axes[3].set_xlabel("Número de trama")
    axes[3].grid(True, alpha=0.35)

    # Forzar xmin a 0
    axes[3].set_xlim(0, n - 1)

    plt.tight_layout()
    fig.savefig(OUTPUT_IMAGE_PATH, dpi=200, bbox_inches="tight")
    print(f"Guardado: {OUTPUT_IMAGE_PATH}")


if __name__ == "__main__":
    main()
