# QNX Neutrino 1 kHz sensor pipeline

**QNX Neutrino 8.0 · message passing · 1 kHz timers · priority scheduling · jitter measurement**

Portfolio project: a multi-process real-time-style pipeline on **QNX SDP 8.0** (QEMU quick-start target), plus a Linux timing comparison.

> I built a **1 kHz** sensor → filter → logger pipeline using Neutrino **synchronous message passing**, measured **1 ms wake jitter** idle vs CPU hogs with **priority separation**, and repeated the timing test on Linux. Work ran in **QEMU** (not bare metal); results are interpreted with that caveat.

---

## Why this matters for QNX / RTOS work

Hands-on with APIs and patterns used on Neutrino systems:

| Area | What I implemented |
|------|--------------------|
| IPC | `name_attach` / `name_open`, `MsgSend` / `MsgReceive` / `MsgReply` |
| Timers | Pulse-driven 1 ms timers (`timer_create`, `SIGEV_PULSE_INIT`, `MsgReceive`) |
| Scheduling | High-prio jitter task (50) vs low-prio CPU hogs (10) via `SchedSet` |
| Design | Keep **disk I/O off the hot path** (dedicated logger; filter replies to sensor first) |
| Measurement | Preallocated buffers; `jitter = t_actual − (t0 + n·1ms)`; CSV + plots |
| Comparison | Same jitter methodology on Linux (`clock_nanosleep` + absolute deadlines) |

---

## Architecture

```text
QNX pipeline
  sensor (1 ms pulses)
      |  MsgSend(imu_sample_t)
      v
  filter (IIR low-pass, α = 0.1)
      |  MsgReply(filter_reply_t)     ← unblocks sensor first
      |  MsgSend(log_record_t)
      v
  logger  →  RAM array  →  pipeline.csv (after N samples)

QNX load test
  sensor_jitter @ prio 50     hog @ prio 10 (×N busy loops)
           \_____________________/
                     |
              jitter histogram

Linux (comparison only)
  clock_nanosleep(TIMER_ABSTIME) ± optional SCHED_FIFO
  nice(10) hogs — same CSV format, no full pipeline
```

Shared protocol: `common/protocol.h`.

---

## Repository layout

```text
common/protocol.h      message structs, attach names
qnx/                   Neutrino programs (cross-built with qcc)
linux/                 Stage 7 host timing programs (gcc)
analysis/              plot_pipeline.py, plot_jitter.py
results/               example CSVs / PNGs from lab runs
Makefile               make (QNX) · make linux
```

---

## Build

**QNX** (from a host with SDP 8.0):

```bash
source ~/qnx800/qnxsdp-env.sh
make
```

**Linux** timing tools:

```bash
make linux
```

---

## Run (QNX VM)

Copy binaries to the Quick Start QEMU target, then:

```bash
# Pipeline — start logger first
./logger &
./filter &
./sensor

# Jitter — idle
./sensor_jitter jitter_idle.csv

# Jitter — under load
./hog & ./hog & ./hog & ./hog &
./sensor_jitter jitter_hogs.csv
slay hog
```

Plot on the host:

```bash
python3 analysis/plot_pipeline.py results/pipeline.csv -o results/pipeline.png
python3 analysis/plot_jitter.py results/jitter_idle.csv -o results/jitter_idle.png
python3 analysis/plot_jitter.py results/jitter_hogs.csv -o results/jitter_hogs.png
```

---

## Results (lab notes)

### Pipeline
Fake ~2 Hz IMU-style signal through a first-order IIR low-pass. Filtered output tracks raw with expected lag/smoothing (`results/pipeline.png`).

### QNX jitter (QEMU)
Idle and hog runs show **millisecond-scale** wake error, often clustered near integer milliseconds. Under hogs, mass shifts further from ideal zero. This is consistent with **virtualized timers and host scheduling**, not a claim of hard real-time on silicon.

### Linux jitter (bare host)
Typical wakes in the **tens–hundreds of microseconds**. Under hogs the main peak was sometimes **tighter** than idle — explained by **CPU C-state exit latency** (loaded cores stay awake). That is power management, not “load improves RT.”

### Fair comparison
QNX-in-QEMU vs Linux-on-host is **not** an apples-to-apples RT bake-off. The Neutrino value in this repo is the **IPC design, priorities, and measurement method**. A fair RT claim would require QNX on real hardware (or a tighter lab setup).

---

## Design choices worth discussing

1. **Filter replies to the sensor before talking to the logger** — sensor is not blocked on logging.
2. **Logger stores in RAM and writes CSV once** — same idea as “no `printf` in the 1 ms path.”
3. **Synchronous Neutrino messaging** — one call does data copy + rendezvous; no shared ring for Stage 4/5.
4. **Absolute periods on Linux** — `TIMER_ABSTIME` avoids drift from repeated relative sleeps.
5. **Honest debugging** — fixed unsigned underflow in fake noise (`(seq % 17) - 8` with `uint32_t`).

---

## Environment

- QNX SDP 8.0 (non-commercial), `qcc -Vgcc_ntox86_64`
- Quick Start Target Image for QEMU (x86_64)
- Host: Linux (Fedora) for cross-compile + Stage 7
- Language: C11

---

## What I’d do next

- Re-run QNX jitter on **real hardware**
- Longer captures, more hog/priority sweeps
- Linux **PREEMPT_RT** / proper FIFO + CPU isolation for a cleaner comparison
- Optional: port the full pipeline to Linux for feature parity (not required for the QNX story)

---

## Resume line

*QNX Neutrino: 1 kHz multi-process sensor pipeline using message passing; 1 ms timer jitter under priority-separated CPU load; Linux timing comparison.*
