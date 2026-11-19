# QNX real-time sensor pipeline

A small multi-process pipeline on QNX Neutrino: a 1 kHz sensor task, a filter, a logger, and CPU hogs, then the same timing measurement on Linux. The question is whether a high-priority task still wakes every 1 ms when lower-priority processes saturate the CPU.

This repository is at **stage 1**: prove the `qcc` cross-compiler and that a binary copied to a QNX virtual machine actually runs there. Timers, message passing, jitter, and the Linux comparison come in later stages.

## What `qcc` is

`qcc` is QNX's compiler driver. It runs on this Linux host and produces a **QNX Neutrino** executable, not a Linux one. The SDP environment script sets that up:

- `QNX_HOST` — host tools, including `qcc`
- `QNX_TARGET` — Neutrino headers and libraries (the sysroot the linker uses)

```bash
source ~/qnx800/qnxsdp-env.sh
```

`-Vgcc_ntox86_64` tells `qcc` which target to build: GCC, Neutrino, x86_64. List the installed specs with `qcc -V`.

`qnx/hello.c` is plain C11. It prints one line and the result of `uname()`. On the QNX VM, `sysname` is `QNX`.

## 1. Install QNX SDP 8.0

Official host support is Ubuntu 20.04/22.04, RHEL 9, and Windows 10/11. This machine is Fedora 44. The toolchain is self-contained and usually still runs here. If the installer errors, stop and report it rather than forcing a different host OS.

Do this as your normal user. Do **not** use `sudo` for the QNX installer or Software Center. `sudo` changes ownership of files under your home directory and breaks later updates.

1. Log in at [myQNX](https://www.qnx.com/account/) and activate a free non-commercial SDP 8.0 license ([get QNX](https://www.qnx.com/getqnx)).
2. Download the Linux **QNX Software Center 2.0** installer: `qnx-setup-<build>-lin.run`.
3. Install it and accept the default directory `~/qnx`:

```bash
chmod a+x qnx-setup-<build>-lin.run
./qnx-setup-<build>-lin.run
```

Relaunch later with `~/qnx/qnxsoftwarecenter/qnxsoftwarecenter`.

4. In Software Center, choose **Add Installation**, then **QNX Software Development Platform 8.0**. Use the **Conservative** installation policy and the default directory `~/qnx800`.
5. On the **Available** tab, search for `quick start` and install **QNX SDP 8.0 Quick Start Target Image for QEMU** (`com.qnx.qnx800.quickstart.qemu`). If it does not appear, turn off **Show only latest version**.
6. So new terminals can find `qcc`, add this to `~/.bashrc`:

```bash
source "$HOME/qnx800/qnxsdp-env.sh"
```

Open a new terminal, or run that `source` line in the current one.

## 2. Build

From this repository:

```bash
source ~/qnx800/qnxsdp-env.sh
qcc -V
make
file build/hello
```

`file` should describe a QNX Neutrino x86-64 executable. `make` refuses to run if `QNX_HOST` is unset, and tells you to source the environment script.

## 3. Run it on the QNX VM

Cross-compiling does not need KVM. Booting the VM does. This host has the Intel KVM module installed, but `/dev/kvm` is missing until the module is loaded, and QEMU is not installed yet. Both need `sudo`:

```bash
sudo modprobe kvm_intel
sudo dnf install qemu-system-x86 qemu-kvm
ls -l /dev/kvm
```

The quick-start image defaults to 8 vCPUs and 4 GB of RAM. This machine has 24 threads and 15 GB of RAM, so that default fits. If the package README or PDF under the image directory disagrees with the commands below, follow the package.

```bash
source ~/qnx800/qnxsdp-env.sh
cd ~/qnx800/images/qemu
./unpack_qemu_image.sh
cd qemu
mkqnximage --run
```

Leave that terminal open. In a second terminal, from the same `qemu` directory:

```bash
source ~/qnx800/qnxsdp-env.sh
mkqnximage --getip
```

Copy and run the binary. These images ship a user `qnxuser` whose initial password is `qnxuser` (and `root` / `root`). Use `qnxuser`:

```bash
scp build/hello qnxuser@<ip>:~/hello
ssh qnxuser@<ip> ./hello
```

Expected output:

```text
hello from QNX pipeline stage 1
sysname=QNX release=... machine=x86_64
```

If SSH reports a MAC or algorithm error, retry with `-o MACs=hmac-sha2-256`. Some QNX `sshd` builds require that algorithm.

Stop the VM from the image directory with `mkqnximage --stop`.

## Stage 1 is done when

- `file build/hello` reports a QNX Neutrino x86-64 binary.
- On the VM, `./hello` prints the greeting and `sysname=QNX`.

Stage 2 is a single process that waits on a 1 ms timer pulse and prints a tick count once per second. It is not in this tree yet.
