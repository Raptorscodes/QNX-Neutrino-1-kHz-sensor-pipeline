# Lab results

Captured CSVs and plots from portfolio runs. Units for jitter are **nanoseconds** (`jitter_ns`); QNX peaks near `1e6`–`3e6` ns are **1–3 ms**.

```text
results/
  qnx/       Neutrino @ QEMU
  linux/     Fedora host
```

| Path | What it is |
|------|------------|
| `qnx/pipeline.{csv,png}` | Sensor → filter LPF (raw vs filtered) |
| `qnx/jitter_idle.*` | 1 ms wake jitter, no hogs |
| `qnx/jitter_hogs.*` | 1 ms wake jitter with CPU hogs |
| `linux/jitter_idle.*` | Same measurement on Linux (idle) |
| `linux/jitter_hogs.*` | Same measurement on Linux (with hogs) |

Re-plot examples:

```bash
python3 analysis/plot_pipeline.py results/qnx/pipeline.csv -o results/qnx/pipeline.png
python3 analysis/plot_jitter.py results/qnx/jitter_idle.csv -o results/qnx/jitter_idle.png
python3 analysis/plot_jitter.py results/linux/jitter_idle.csv -o results/linux/jitter_idle.png
```
