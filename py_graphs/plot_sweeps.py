import re
import argparse
from pathlib import Path
import matplotlib.pyplot as plt


# --------- Parsing ---------
RE_PARAM = re.compile(r"^\s*([-+]?\d+(?:\.\d+)?)\s*$")
RE_UV_AS_V = re.compile(r"Unvoiced\s+frames\s+as\s+voiced:\s*\d+\s*/\s*\d+\s*\(\s*([0-9]+(?:\.[0-9]+)?)\s*%\s*\)")
RE_V_AS_UV = re.compile(r"Voiced\s+frames\s+as\s+unvoiced:\s*\d+\s*/\s*\d+\s*\(\s*([0-9]+(?:\.[0-9]+)?)\s*%\s*\)")
RE_GROSS = re.compile(r"Gross\s+voiced\s+errors.*?\(\s*([0-9]+(?:\.[0-9]+)?)\s*%\s*\)")
RE_MSE = re.compile(r"MSE\s+of\s+fine\s+errors:\s*([0-9]+(?:\.[0-9]+)?)\s*%")
RE_TOTAL_SCORE = re.compile(r"===>\s*TOTAL:\s*([0-9]+(?:\.[0-9]+)?)\s*%")


def parse_results_txt(txt_path: str):
    """
    Returns list of dicts:
    [{"param": float, "uv_as_v": float, "v_as_uv": float, "gross": float, "mse": float, "score": float}, ...]
    """
    rows = []
    cur = {"param": None, "uv_as_v": None, "v_as_uv": None, "gross": None, "mse": None, "score": None}

    def flush_if_complete():
        nonlocal cur
        if all(cur[k] is not None for k in ("param", "uv_as_v", "v_as_uv", "gross", "mse", "score")):
            rows.append(cur)
            cur = {"param": None, "uv_as_v": None, "v_as_uv": None, "gross": None, "mse": None, "score": None}

    with open(txt_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip("\n")

            m = RE_PARAM.match(line.strip())
            if m:
                flush_if_complete()
                cur["param"] = float(m.group(1))
                continue

            m = RE_UV_AS_V.search(line)
            if m:
                cur["uv_as_v"] = float(m.group(1))
                continue

            m = RE_V_AS_UV.search(line)
            if m:
                cur["v_as_uv"] = float(m.group(1))
                continue

            m = RE_GROSS.search(line)
            if m:
                cur["gross"] = float(m.group(1))
                continue

            m = RE_MSE.search(line)
            if m:
                cur["mse"] = float(m.group(1))
                continue

            m = RE_TOTAL_SCORE.search(line)
            if m:
                cur["score"] = float(m.group(1))
                flush_if_complete()
                continue

    flush_if_complete()
    rows.sort(key=lambda d: d["param"])
    return rows


# --------- Plotting ---------
def plot_metrics(rows, title: str, out_png: str):
    params = [r["param"] for r in rows]
    uv_as_v = [r["uv_as_v"] for r in rows]
    v_as_uv = [r["v_as_uv"] for r in rows]
    gross = [r["gross"] for r in rows]
    mse = [r["mse"] for r in rows]
    score = [r["score"] for r in rows]

    fig, axes = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

    # Subplot 1: all error metrics
    axes[0].plot(params, uv_as_v, marker="o", label="Unvoiced as voiced (%)")
    axes[0].plot(params, v_as_uv, marker="o", label="Voiced as unvoiced (%)")
    axes[0].plot(params, gross, marker="o", label="Gross voiced errors (%)")
    axes[0].plot(params, mse, marker="o", label="MSE of fine errors (%)")
    axes[0].set_ylabel("Metric (%)")
    axes[0].set_title(title)
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    # Subplot 2: TOTAL score
    axes[1].plot(params, score, marker="o", label="TOTAL score (%)")
    axes[1].set_xlabel("Parameter value")
    axes[1].set_ylabel("Score (%)")
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    fig.tight_layout()
    fig.savefig(out_png, dpi=200)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clip-txt", required=True, help="Path to txt file sweeping center-clipping threshold")
    ap.add_argument("--med-txt", required=True, help="Path to txt file sweeping median filter window size")
    ap.add_argument("--out-dir", default="plots", help="Output directory for pngs")
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    rows_clip = parse_results_txt(args.clip_txt)
    if not rows_clip:
        raise RuntimeError(f"No valid blocks parsed from {args.clip_txt}")
    plot_metrics(rows_clip, "Sweep: center-clipping threshold", str(out_dir / "sweep_center_clipping.png"))

    rows_med = parse_results_txt(args.med_txt)
    if not rows_med:
        raise RuntimeError(f"No valid blocks parsed from {args.med_txt}")
    plot_metrics(rows_med, "Sweep: median filter window size", str(out_dir / "sweep_median_window.png"))

    print("Saved:")
    print(" -", out_dir / "sweep_center_clipping.png")
    print(" -", out_dir / "sweep_median_window.png")


if __name__ == "__main__":
    main()
