# Cross-compile QNX Neutrino x86_64 binaries with qcc.
#
#   source ~/qnx800/qnxsdp-env.sh
#   make
#   make build/sensor_jitter
#
# -Vgcc_ntox86_64 means "GCC, Neutrino OS, x86_64".

QCC      ?= qcc
QCC_SPEC ?= gcc_ntox86_64
CFLAGS   ?= -std=c11 -Wall -Wextra -O2

.PHONY: all clean check-env

all: build/hello build/timer_tick build/sensor_jitter

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

build/timer_tick: qnx/timer_tick.c | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/timer_tick.c

build/sensor_jitter: qnx/sensor_jitter.c | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/sensor_jitter.c

clean:
	rm -rf build
