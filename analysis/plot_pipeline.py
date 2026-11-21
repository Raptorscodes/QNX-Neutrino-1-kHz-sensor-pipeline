#!/usr/bin/env python3
"""
Stage 5 — plot raw vs filtered accel from the logger CSV.

Expected CSV columns:
  seq,timestamp_ns,accel_z,filtered_z
(header row required)

Example:
  python3 analysis/plot_pipeline.py results/pipeline.csv -o results/pipeline.png
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot pipeline raw vs filtered CSV")
    parser.add_argument("csv_path", type=Path, help="path to pipeline CSV")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="optional output image path (e.g. results/pipeline.png)",
    )
    args = parser.parse_args()

    if not args.csv_path.is_file():
        print(f"file not found: {args.csv_path}", file=sys.stderr)
        return 1

    data = np.loadtxt(args.csv_path, delimiter=",", skiprows=1)
    if data.ndim == 1:
        data = data.reshape(1, -1)

    seq = data[:, 0]
    raw = data[:, 2]
    filt = data[:, 3]

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(seq, raw, label="accel_z (raw)", linewidth=0.8)
    ax.plot(seq, filt, label="filtered_z", linewidth=1.0)
    ax.set_xlabel("seq")
    ax.set_ylabel("acceleration")
    ax.set_title("Stage 5 — raw vs filtered")
    ax.legend(loc="upper right")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()

    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(args.output, dpi=150)
        print(f"wrote {args.output}")
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
