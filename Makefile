# Stage 1 only: cross-compile hello.c into a QNX Neutrino x86_64 binary.
# Later stages add the sensor, filter, logger, and the Linux comparison.
#
# qcc is QNX's compiler driver. It is not on PATH until you source the
# SDP environment (that script exports QNX_HOST and QNX_TARGET):
#
#   source ~/qnx800/qnxsdp-env.sh
#
# -V selects the target. gcc_ntox86_64 means "GCC, Neutrino OS, x86_64".
# Confirm the name with `qcc -V` after installing SDP 8.0.

QCC      ?= qcc
QCC_SPEC ?= gcc_ntox86_64
CFLAGS   ?= -std=c11 -Wall -Wextra -O2

.PHONY: all clean check-env

all: build/hello

# Order-only so a missing toolchain fails the build, without forcing
# a rebuild on every `make` once build/hello already exists.
check-env:
	@test -n "$(QNX_HOST)" || { \
		echo "QNX_HOST is unset. Run: source ~/qnx800/qnxsdp-env.sh"; \
		exit 1; \
	}
	@command -v $(QCC) >/dev/null 2>&1 || { \
		echo "qcc not on PATH. Run: source ~/qnx800/qnxsdp-env.sh"; \
		exit 1; \
	}

build/hello: qnx/hello.c | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/hello.c

clean:
	rm -rf build
