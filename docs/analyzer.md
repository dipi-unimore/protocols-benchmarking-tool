# Analyzer

The `analyzer/` directory contains a standalone Python CLI tool for visualizing and comparing benchmark runs. It is completely independent of the C++ build system.

**Requirements:** Python 3.11+, [uv](https://docs.astral.sh/uv/)

## Setup

```bash
cd analyzer
uv sync
```

## Usage

```bash
# List available runs without generating plots
uv run analyzer --list-only

# Interactive comparison (default: reads from ../results or $PBT_OUTPUT_DIR)
uv run analyzer

# Override results directory
uv run analyzer --results-dir /path/to/results

# Override plot output parent directory (default: ./plots/)
uv run analyzer --out-dir /tmp/analysis
```

### Workflow

1. The tool scans the results directory and displays a numbered table sorted by protocol → serializer → compression.
2. Runs missing `config.json`, `summary.json`, or `packets.csv` are shown as `incomplete` and cannot be selected.
3. You enter a comma-separated list of run numbers (e.g. `1,3`).
4. The tool validates that all selected runs share the same `message_rate_hz`, `payload_size_bytes`, and `duration_s`. Mismatches cause a hard exit with a diff table.
5. Figures are generated in a timestamped subfolder and a summary is printed.

### Results Directory Resolution

Priority (highest first):

1. `--results-dir` CLI flag
2. `PBT_OUTPUT_DIR` environment variable
3. `../results` (relative to `analyzer/`)

## Output

Each session creates a timestamped subfolder:

```
plots/20260625_143022_500hz_512B_10s/
├── hist_e2e.png / .pdf
├── hist_serialization.png / .pdf
├── hist_compression.png / .pdf
├── hist_transport.png / .pdf
├── hist_decompression.png / .pdf
├── hist_deserialization.png / .pdf
├── scatter_e2e.png / .pdf
├── scatter_serialization.png / .pdf
├── scatter_compression.png / .pdf
├── scatter_transport.png / .pdf
├── scatter_decompression.png / .pdf
├── scatter_deserialization.png / .pdf
├── latency_summary.png / .pdf
├── reliability_throughput.png / .pdf
├── ts_e2e.png / .pdf
├── ts_jitter.png / .pdf
└── ts_throughput.png / .pdf
```

34 files total (17 figures × PNG + PDF).

## Plot Descriptions

### Distribution plots (per latency dimension)

| File | Source | Description |
|------|--------|-------------|
| `hist_{dim}` | `packets.csv` | Overlaid semi-transparent histograms (alpha 0.6); one distribution per run. Dimensions with all-zero values (e.g. `serialization` when `serializer=none`) are annotated as N/A. |
| `scatter_{dim}` | `packets.csv` | Every packet plotted as a point: x = elapsed seconds from first received packet, y = latency µs. All points rendered (no downsampling). |

Latency dimensions: `e2e`, `serialization`, `compression`, `transport`, `decompression`, `deserialization`.

### Summary & reliability plots

| File | Source | Description |
|------|--------|-------------|
| `latency_summary` | `summary.json` | Grouped bar chart: p50 / p95 / p99 / p99.9 for each latency dimension, one group of bars per run. |
| `reliability_throughput` | `summary.json` | 4-panel bar chart: `out_of_order_count`, `packet_loss_pct`, `throughput_bytes_per_sec`, `throughput_msgs_per_sec`. |

### Time series plots

| File | Source | Description |
|------|--------|-------------|
| `ts_e2e` | `packets.csv` | E2E delay (µs) vs elapsed time (s). Raw scatter (low alpha) + rolling median overlay (window = 5% of packet count). |
| `ts_jitter` | `packets.csv` | Jitter (µs) vs elapsed time. Same scatter + rolling median style. |
| `ts_throughput` | `packets.csv` | Throughput in 1-second bins. Twin y-axes: bytes/s (left, solid) and msg/s (right, dashed). |

### Visual conventions

- Colors: `tab10` palette, one color per run.
- Legend labels: `protocol/serializer/compression` (e.g. `tcp/none/none`).
- Figure size: 12 × 7 inches, 150 DPI PNG + vector PDF.
- All figures are saved only (no interactive window).

## Comparability Rules

Two runs are **Comparable** if and only if they share the same values for:

- `message_rate_hz`
- `payload_size_bytes`
- `duration_s`

Selecting non-comparable runs aborts with exit code 1 and prints a diff table identifying the mismatched parameters.
