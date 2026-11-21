# Cross-compile QNX Neutrino x86_64 binaries with qcc.
#
#   source ~/qnx800/qnxsdp-env.sh
#   make
#   make build/logger build/filter build/sensor
#
# Stage 5 on the VM (logger first, then filter, then sensor):
#   ./logger &
#   ./filter &
#   ./sensor
#   # copy pipeline.csv off the VM, then:
#   # python3 analysis/plot_pipeline.py results/pipeline.csv -o results/pipeline.png

QCC      ?= qcc
QCC_SPEC ?= gcc_ntox86_64
CFLAGS   ?= -std=c11 -Wall -Wextra -O2 -Icommon
LDLIBS_SENSOR ?= -lm

.PHONY: all clean check-env

all: build/hello build/timer_tick build/sensor_jitter build/logger build/filter build/sensor

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

build/logger: qnx/logger.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/logger.c

build/filter: qnx/filter.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/filter.c

build/sensor: qnx/sensor.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/sensor.c $(LDLIBS_SENSOR)

clean:
	rm -rf build
