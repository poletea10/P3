import numpy as np
import matplotlib.pyplot as plt

output_path = "img/temp_vs_autocorrelacion.png"

def design_formant_resonator(fs, f0, bw):
    """
    Diseña un resonador IIR de 2º orden (par de polos) para un formante.
    Retorna coeficientes a (denominador) y b (numerador) para y[n] = b0*x[n] - a1*y[n-1] - a2*y[n-2]
    """
    r = np.exp(-np.pi * bw / fs)
    w0 = 2 * np.pi * f0 / fs
    a1 = -2 * r * np.cos(w0)
    a2 = r**2
    # Ganancia simple para que no se apague demasiado (no es un diseño "perfecto", pero suena/parece realista)
    b0 = 1 - r
    return b0, a1, a2

def apply_resonator(x, b0, a1, a2):
    y = np.zeros_like(x)
    y1, y2 = 0.0, 0.0
    for n in range(len(x)):
        y0 = b0 * x[n] - a1 * y1 - a2 * y2
        y[n] = y0
        y2, y1 = y1, y0
    return y

def voiced_phoneme_segment(fs=16000, dur_ms=30.0, f0_hz=140.0, seed=0):
    """
    Genera un fonema sonoro sintético (tipo vocal) de ~30 ms:
    - excitación: tren de impulsos (pitch) + pequeña aspiración
    - filtrado: resonadores de formantes (F1,F2,F3)
    """
    rng = np.random.default_rng(seed)

    N = int(round(fs * (dur_ms / 1000.0)))
    t = np.arange(N) / fs

    # Pitch
    T0 = 1.0 / f0_hz
    T0_samp = int(round(T0 * fs))

    # Excitación: tren de impulsos
    exc = np.zeros(N)
    exc[::T0_samp] = 1.0

    # Suavizar un poco cada impulso para parecerse más a un pulso glotal (muy simple)
    # (convolución con una ventana corta asimétrica)
    glot = np.concatenate([np.linspace(0, 1, 8, endpoint=False), np.linspace(1, 0, 24)])
    glot = glot / np.sum(glot)
    exc = np.convolve(exc, glot, mode="same")

    # Añadir un toque de "aspiración" (ruido leve)
    exc += 0.02 * rng.standard_normal(N)

    # Envolvente (ataque/decay suave) para que no sea un bloque perfecto
    env = np.ones(N)
    attack = int(0.15 * N)
    release = int(0.20 * N)
    if attack > 1:
        env[:attack] = np.linspace(0, 1, attack)
    if release > 1:
        env[-release:] = np.linspace(1, 0.7, release)
    exc *= env

    # Formantes típicos (aprox) para una vocal tipo /a/
    formants = [
        (730, 80),   # F1, BW
        (1090, 90),  # F2
        (2440, 120)  # F3
    ]

    y = exc.copy()
    for f, bw in formants:
        b0, a1, a2 = design_formant_resonator(fs, f, bw)
        y = apply_resonator(y, b0, a1, a2)

    # Normalizar a [-1,1] aprox
    y /= (np.max(np.abs(y)) + 1e-12)
    return t, y, f0_hz, T0_samp

def autocorr(x):
    """
    Autocorrelación (no sesgada) vía correlación directa:
    devuelve r[k] para k>=0 (lags no-negativos), normalizada con r[0]=1.
    """
    x = x - np.mean(x)
    r_full = np.correlate(x, x, mode="full")
    r = r_full[len(x)-1:]  # lags >= 0
    r = r / (r[0] + 1e-12)
    return r

def find_first_secondary_max(r, fs, f0_guess=None):
    """
    Busca el primer máximo secundario "claro" en la autocorrelación:
    - ignora el pico principal en lag=0
    - restringe el rango plausible de pitch (por defecto 60–400 Hz)
    - devuelve lag_peak (muestras)
    """
    # Rango típico de pitch humano (puedes ajustarlo)
    fmin, fmax = 60.0, 400.0
    lag_min = int(np.floor(fs / fmax))
    lag_max = int(np.ceil(fs / fmin))
    lag_max = min(lag_max, len(r)-1)

    # Opcional: si tienes una estimación de f0, acota alrededor
    if f0_guess is not None:
        T0 = fs / f0_guess
        lag_min = max(lag_min, int(0.6 * T0))
        lag_max = min(lag_max, int(1.6 * T0))

    rr = r[lag_min:lag_max+1]
    lag_candidates = np.arange(lag_min, lag_max+1)

    # Máximo en esa ventana
    idx = np.argmax(rr)
    lag_peak = lag_candidates[idx]
    return lag_peak

if __name__ == "__main__":
    fs = 16000
    dur_ms = 30.0

    # Generar fonema sonoro (puedes cambiar f0_hz para ver otros pitches)
    t, x, f0_hz, T0_samp_true = voiced_phoneme_segment(fs=fs, dur_ms=dur_ms, f0_hz=140.0, seed=1)

    # Autocorrelación
    r = autocorr(x)

    # Primer máximo secundario (asociado a pitch)
    lag_peak = find_first_secondary_max(r, fs, f0_guess=f0_hz)

    # Medidas
    T0_time_true_ms = 1000.0 * (T0_samp_true / fs)
    T0_from_ac_ms = 1000.0 * (lag_peak / fs)

    # Para el subplot temporal: marcar un periodo de pitch
    # Elegimos un punto de inicio donde se vea bien (cerca del centro)
    start = int(0.007 * fs)  # 10 ms
    end = min(start + T0_samp_true, len(x)-1)

    # Plot
    fig, axes = plt.subplots(2, 1, figsize=(10, 6), constrained_layout=True)

    # --- Subplot 1: señal temporal ---
    ax = axes[0]
    ax.plot(t * 1000, x, linewidth=1.2)
    ax.set_title("Señal temporal (≈30 ms) de fonema sonoro + período de pitch")
    ax.set_xlabel("Tiempo [ms]")
    ax.set_ylabel("Amplitud (normalizada)")
    ax.grid(True, alpha=0.3)

    # Marcar un periodo de pitch en el tiempo
    ax.axvline(t[start] * 1000, linestyle="--")
    ax.axvline(t[end] * 1000, linestyle="--")
    ax.annotate(
        f"Período (pitch) ≈ {T0_time_true_ms:.2f} ms\n({T0_samp_true} muestras)",
        xy=((t[start] + t[end]) * 500, 0.8),
        xycoords=("data", "data"),
        ha="center",
        va="center",
        bbox=dict(boxstyle="round", alpha=0.15)
    )

    # Altura a la que dibujar la línea roja (ligeramente por debajo del texto)
    y_arrow = 0.55 * np.max(x)

    ax.annotate(
        "",
        xy=(t[end] * 1000, y_arrow),
        xytext=(t[start] * 1000, y_arrow),
        arrowprops=dict(
            arrowstyle="<->",
            color="red",
            linewidth=2
        )
    )

    ax.set_xlim(0, dur_ms)

    # --- Subplot 2: autocorrelación ---
    ax = axes[1]
    lags = np.arange(len(r))
    ax.plot(lags, r, linewidth=1.2)
    ax.set_title("Autocorrelación normalizada")
    ax.set_xlabel("Lag [muestras]")
    ax.set_ylabel("r[lag] (normalizada)")
    ax.grid(True, alpha=0.3)

    # Marcar el máximo secundario
    ax.plot(lag_peak, r[lag_peak], marker="o")
    ax.axvline(lag_peak, linestyle="--")
    ax.annotate(
        f"lag = {lag_peak} muestras\n≈ {T0_from_ac_ms:.2f} ms\n⇒ f0 ≈ {fs/lag_peak:.1f} Hz",
        xy=(lag_peak, r[lag_peak]),
        xytext=(lag_peak+30, 0.6),
        textcoords="data",
        arrowprops=dict(arrowstyle="->"),
        bbox=dict(boxstyle="round", alpha=0.15)
    )

    ax.set_xlim(0, lags[-1])

    # Mostrar comparación numérica en consola
    print("=== Comparación pitch ===")
    print(f"Pitch impuesto: f0 = {f0_hz:.2f} Hz  => T0 = {T0_samp_true} muestras = {T0_time_true_ms:.2f} ms")
    print(f"Desde autocorr: lag_peak = {lag_peak} muestras = {T0_from_ac_ms:.2f} ms  => f0 ≈ {fs/lag_peak:.2f} Hz")

    plt.savefig(
        output_path,
        dpi=300,
        bbox_inches="tight"
    )
