from __future__ import annotations

from pathlib import Path
from typing import Callable

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import pandas as pd

from .results import RunResult

_FIG_W, _FIG_H = 12, 7
_DPI = 150
_ALPHA_HIST = 0.6
_ALPHA_SCATTER = 0.25

_LATENCY_DIMS = [
    ("e2e", "e2e_delay_us", "End-to-End Delay"),
    ("serialization", "serialization_us", "Serialization"),
    ("compression", "compression_us", "Compression"),
    ("transport", "transport_us", "Transport"),
    ("decompression", "decompression_us", "Decompression"),
    ("deserialization", "deserialization_us", "Deserialization"),
]

_PERCENTILES = ("p50_us", "p95_us", "p99_us", "p999_us")
_PERCENTILE_LABELS = ("p50", "p95", "p99", "p99.9")


def _na_label(runs: list[RunResult], col: str) -> str | None:
    """Return annotation string when all values for col are zero/NaN across all runs."""
    for run in runs:
        df = run.load_packets()
        if col in df.columns and df[col].abs().max() > 0:
            return None
    # Derive which dimension is absent from config
    cfg = runs[0].config
    if "serialization" in col or "deserialization" in col:
        return f"N/A (serializer={cfg.get('serializer', 'none')})"
    if "compression" in col or "decompression" in col:
        return f"N/A (compression={cfg.get('compression', 'none')})"
    return "N/A"


def _elapsed_s(df: pd.DataFrame) -> pd.Series:
    t0 = df["ts_received_ns"].iloc[0]
    return (df["ts_received_ns"] - t0) / 1e9


def _save(fig: plt.Figure, out_dir: Path, stem: str) -> list[Path]:
    paths = []
    for ext in ("png", "pdf"):
        p = out_dir / f"{stem}.{ext}"
        fig.savefig(p, dpi=_DPI if ext == "png" else None, bbox_inches="tight")
        paths.append(p)
    plt.close(fig)
    return paths


def _colors(n: int) -> list:
    cmap = plt.get_cmap("tab10")
    return [cmap(i) for i in range(n)]


# ── Histogram ─────────────────────────────────────────────────────────────────

def _plot_hist(runs: list[RunResult], dim_key: str, col: str, title: str, out_dir: Path) -> list[Path]:
    fig, ax = plt.subplots(figsize=(_FIG_W, _FIG_H))
    colors = _colors(len(runs))
    na = _na_label(runs, col)

    if na:
        ax.text(0.5, 0.5, na, transform=ax.transAxes,
                ha="center", va="center", fontsize=14, color="gray")
    else:
        for run, color in zip(runs, colors):
            df = run.load_packets()
            if col not in df.columns:
                continue
            data = df[col].dropna()
            ax.hist(data, bins=60, alpha=_ALPHA_HIST, color=color, label=run.legend, edgecolor="none")

    ax.set_xlabel(f"{title} (µs)")
    ax.set_ylabel("Count")
    ax.set_title(f"{title} Distribution")
    if not na:
        ax.legend()
    fig.tight_layout()
    return _save(fig, out_dir, f"hist_{dim_key}")


# ── Scatter ────────────────────────────────────────────────────────────────────

def _plot_scatter(runs: list[RunResult], dim_key: str, col: str, title: str, out_dir: Path) -> list[Path]:
    fig, ax = plt.subplots(figsize=(_FIG_W, _FIG_H))
    colors = _colors(len(runs))
    na = _na_label(runs, col)

    if na:
        ax.text(0.5, 0.5, na, transform=ax.transAxes,
                ha="center", va="center", fontsize=14, color="gray")
    else:
        for run, color in zip(runs, colors):
            df = run.load_packets()
            if col not in df.columns:
                continue
            t = _elapsed_s(df)
            ax.scatter(t, df[col], s=4, alpha=_ALPHA_SCATTER, color=color, label=run.legend, linewidths=0)

    ax.set_xlabel("Elapsed time (s)")
    ax.set_ylabel(f"{title} (µs)")
    ax.set_title(f"{title} per Packet")
    if not na:
        ax.legend(markerscale=3)
    fig.tight_layout()
    return _save(fig, out_dir, f"scatter_{dim_key}")


# ── Latency summary bar chart ──────────────────────────────────────────────────

def _plot_latency_summary(runs: list[RunResult], out_dir: Path) -> list[Path]:
    dims = [d[0] for d in _LATENCY_DIMS]
    n_dims = len(dims)
    n_pct = len(_PERCENTILES)
    n_runs = len(runs)
    colors = _colors(n_runs)

    fig, axes = plt.subplots(1, n_dims, figsize=(n_dims * 2.5, _FIG_H), sharey=False)
    if n_dims == 1:
        axes = [axes]

    x = np.arange(n_pct)
    width = 0.8 / n_runs

    for ax, dim in zip(axes, dims):
        for i, (run, color) in enumerate(zip(runs, colors)):
            lat = (run.summary or {}).get("latency", {}).get(dim, {})
            vals = [lat.get(p, 0.0) for p in _PERCENTILES]
            offset = (i - (n_runs - 1) / 2) * width
            bars = ax.bar(x + offset, vals, width, color=color, label=run.legend)
        ax.set_xticks(x)
        ax.set_xticklabels(_PERCENTILE_LABELS, fontsize=8)
        ax.set_title(dim, fontsize=9)
        ax.set_ylabel("µs")
        ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{v:.0f}"))

    handles = [plt.Rectangle((0, 0), 1, 1, color=colors[i]) for i in range(n_runs)]
    labels = [r.legend for r in runs]
    fig.legend(handles, labels, loc="upper center", ncol=n_runs, bbox_to_anchor=(0.5, 1.02))
    fig.suptitle("Latency Summary (p50 / p95 / p99 / p99.9)", y=1.05)
    fig.tight_layout()
    return _save(fig, out_dir, "latency_summary")


# ── Reliability + throughput bar chart ─────────────────────────────────────────

def _plot_reliability_throughput(runs: list[RunResult], out_dir: Path) -> list[Path]:
    metrics = [
        ("out_of_order_count", "Out-of-Order Count", "packets"),
        ("packet_loss_pct", "Packet Loss", "%"),
        ("throughput_bytes_per_sec", "Throughput", "B/s"),
        ("throughput_msgs_per_sec", "Throughput", "msg/s"),
    ]
    colors = _colors(len(runs))
    fig, axes = plt.subplots(1, 4, figsize=(_FIG_W, _FIG_H))
    x = np.arange(len(runs))

    for ax, (key, title, unit) in zip(axes, metrics):
        vals = [(run.summary or {}).get(key, 0.0) for run in runs]
        bars = ax.bar(x, vals, color=colors[:len(runs)])
        ax.set_title(title, fontsize=9)
        ax.set_ylabel(unit, fontsize=8)
        ax.set_xticks(x)
        ax.set_xticklabels([r.legend for r in runs], rotation=30, ha="right", fontsize=7)

    fig.suptitle("Reliability & Throughput")
    fig.tight_layout()
    return _save(fig, out_dir, "reliability_throughput")


# ── Time series: e2e ──────────────────────────────────────────────────────────

def _plot_ts_e2e(runs: list[RunResult], out_dir: Path) -> list[Path]:
    fig, ax = plt.subplots(figsize=(_FIG_W, _FIG_H))
    colors = _colors(len(runs))
    for run, color in zip(runs, colors):
        df = run.load_packets()
        t = _elapsed_s(df)
        y = df["e2e_delay_us"]
        ax.scatter(t, y, s=4, alpha=_ALPHA_SCATTER, color=color, linewidths=0)
        win = max(1, len(df) // 20)
        med = y.rolling(win, min_periods=1, center=True).median()
        ax.plot(t, med, color=color, linewidth=1.5, label=run.legend)
    ax.set_xlabel("Elapsed time (s)")
    ax.set_ylabel("End-to-End Delay (µs)")
    ax.set_title("End-to-End Delay over Time")
    ax.legend()
    fig.tight_layout()
    return _save(fig, out_dir, "ts_e2e")


# ── Time series: jitter ────────────────────────────────────────────────────────

def _plot_ts_jitter(runs: list[RunResult], out_dir: Path) -> list[Path]:
    fig, ax = plt.subplots(figsize=(_FIG_W, _FIG_H))
    colors = _colors(len(runs))
    for run, color in zip(runs, colors):
        df = run.load_packets()
        t = _elapsed_s(df)
        y = df["jitter_us"]
        ax.scatter(t, y, s=4, alpha=_ALPHA_SCATTER, color=color, linewidths=0)
        win = max(1, len(df) // 20)
        med = y.rolling(win, min_periods=1, center=True).median()
        ax.plot(t, med, color=color, linewidth=1.5, label=run.legend)
    ax.set_xlabel("Elapsed time (s)")
    ax.set_ylabel("Jitter (µs)")
    ax.set_title("Jitter over Time")
    ax.legend()
    fig.tight_layout()
    return _save(fig, out_dir, "ts_jitter")


# ── Time series: throughput ────────────────────────────────────────────────────

def _plot_ts_throughput(runs: list[RunResult], out_dir: Path) -> list[Path]:
    fig, ax1 = plt.subplots(figsize=(_FIG_W, _FIG_H))
    ax2 = ax1.twinx()
    colors = _colors(len(runs))

    for run, color in zip(runs, colors):
        df = run.load_packets()
        t_s = _elapsed_s(df).values
        payload = df["payload_size_bytes"].values

        # 1-second bins
        t_min, t_max = t_s.min(), t_s.max()
        if t_max <= t_min:
            continue
        bins = np.arange(t_min, t_max + 1.0, 1.0)
        bin_idx = np.digitize(t_s, bins) - 1
        n_bins = len(bins) - 1
        bps = np.zeros(n_bins)
        mps = np.zeros(n_bins)
        for b in range(n_bins):
            mask = bin_idx == b
            bps[b] = payload[mask].sum()
            mps[b] = mask.sum()
        bin_centers = bins[:-1] + 0.5

        ax1.plot(bin_centers, bps, color=color, linewidth=1.5, label=run.legend)
        ax2.plot(bin_centers, mps, color=color, linewidth=1.0, linestyle="--")

    ax1.set_xlabel("Elapsed time (s)")
    ax1.set_ylabel("Throughput (B/s)")
    ax2.set_ylabel("Throughput (msg/s)")
    ax1.set_title("Throughput over Time (solid=B/s, dashed=msg/s)")
    ax1.legend(loc="upper left")
    fig.tight_layout()
    return _save(fig, out_dir, "ts_throughput")


# ── Public entry points ────────────────────────────────────────────────────────

def generate_all(
    runs: list[RunResult],
    out_dir: Path,
    progress_cb: Callable[[str], None] | None = None,
) -> list[Path]:
    def tick(name: str) -> None:
        if progress_cb:
            progress_cb(name)

    out: list[Path] = []

    for dim_key, col, title in _LATENCY_DIMS:
        tick(f"hist_{dim_key}")
        out += _plot_hist(runs, dim_key, col, title, out_dir)
        tick(f"scatter_{dim_key}")
        out += _plot_scatter(runs, dim_key, col, title, out_dir)

    tick("latency_summary")
    out += _plot_latency_summary(runs, out_dir)

    tick("reliability_throughput")
    out += _plot_reliability_throughput(runs, out_dir)

    tick("ts_e2e")
    out += _plot_ts_e2e(runs, out_dir)

    tick("ts_jitter")
    out += _plot_ts_jitter(runs, out_dir)

    tick("ts_throughput")
    out += _plot_ts_throughput(runs, out_dir)

    return out
