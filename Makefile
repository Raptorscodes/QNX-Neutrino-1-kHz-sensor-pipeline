# QNX cross-compile (Stages 1–6) and Linux host build (Stage 7).
#
# QNX:
#   source ~/qnx800/qnxsdp-env.sh
#   make
#   make build/hog build/sensor_jitter
#
# Stage 6 on the QNX VM:
#   ./hog & ./hog & ./hog & ./hog &
#   ./sensor_jitter jitter_hogs.csv
#   slay hog
#
# Stage 7 on the Linux host (no qcc):
#   make linux
#   ./build/linux/sensor_jitter results/linux_jitter_idle.csv
#   ./build/linux/hog & ./build/linux/hog & ./build/linux/hog & ./build/linux/hog &
#   ./build/linux/sensor_jitter results/linux_jitter_hogs.csv
#   killall hog
#   python3 analysis/plot_jitter.py results/linux_jitter_idle.csv -o results/linux_jitter_idle.png
#   python3 analysis/plot_jitter.py results/linux_jitter_hogs.csv -o results/linux_jitter_hogs.png

QCC      ?= qcc
QCC_SPEC ?= gcc_ntox86_64
CFLAGS   ?= -std=c11 -Wall -Wextra -O2 -Icommon
LDLIBS_SENSOR ?= -lm

HOST_CC      ?= gcc
HOST_CFLAGS  ?= -std=c11 -Wall -Wextra -O2

.PHONY: all clean check-env linux

all: build/hello build/timer_tick build/sensor_jitter build/logger build/filter build/sensor build/hog

linux: build/linux/hog build/linux/sensor_jitter

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

build/hog: qnx/hog.c | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/hog.c

build/logger: qnx/logger.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/logger.c

build/filter: qnx/filter.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/filter.c

build/sensor: qnx/sensor.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/sensor.c $(LDLIBS_SENSOR)

build/linux/hog: linux/hog.c
	mkdir -p build/linux
	$(HOST_CC) $(HOST_CFLAGS) -o $@ linux/hog.c

build/linux/sensor_jitter: linux/sensor_jitter.c
	mkdir -p build/linux
	$(HOST_CC) $(HOST_CFLAGS) -o $@ linux/sensor_jitter.c -lpthread

clean:
	rm -rf build
