# QNX cross-compile (Stages 1–6) and Linux host build (Stage 7).
#
#   source ~/qnx800/qnxsdp-env.sh && make
#   make linux

QCC      ?= qcc
QCC_SPEC ?= gcc_ntox86_64
CFLAGS   ?= -std=c11 -Wall -Wextra -O2 -Icommon -Iqnx
LDLIBS_SENSOR ?= -lm

HOST_CC      ?= gcc
HOST_CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Icommon

PULSE_TIMER_SRC = qnx/pulse_timer.c
SCHED_UTIL_SRC  = qnx/sched_util.c
JITTER_CSV_SRC  = common/jitter_csv.c

.PHONY: all clean check-env linux

all: build/hello build/timer_tick build/sensor_jitter build/logger \
     build/filter build/sensor build/hog

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

build/timer_tick: qnx/timer_tick.c $(PULSE_TIMER_SRC) qnx/pulse_timer.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/timer_tick.c $(PULSE_TIMER_SRC)

build/sensor_jitter: qnx/sensor_jitter.c $(PULSE_TIMER_SRC) $(SCHED_UTIL_SRC) \
		$(JITTER_CSV_SRC) qnx/pulse_timer.h qnx/sched_util.h \
		common/jitter_csv.h common/time_util.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/sensor_jitter.c \
		$(PULSE_TIMER_SRC) $(SCHED_UTIL_SRC) $(JITTER_CSV_SRC)

build/hog: qnx/hog.c $(SCHED_UTIL_SRC) qnx/sched_util.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/hog.c $(SCHED_UTIL_SRC)

build/logger: qnx/logger.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/logger.c

build/filter: qnx/filter.c common/protocol.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/filter.c

build/sensor: qnx/sensor.c $(PULSE_TIMER_SRC) common/protocol.h \
		qnx/pulse_timer.h common/time_util.h | check-env
	mkdir -p build
	$(QCC) -V$(QCC_SPEC) $(CFLAGS) -o $@ qnx/sensor.c $(PULSE_TIMER_SRC) \
		$(LDLIBS_SENSOR)

build/linux/hog: linux/hog.c
	mkdir -p build/linux
	$(HOST_CC) $(HOST_CFLAGS) -o $@ linux/hog.c

build/linux/sensor_jitter: linux/sensor_jitter.c $(JITTER_CSV_SRC) \
		common/jitter_csv.h common/time_util.h
	mkdir -p build/linux
	$(HOST_CC) $(HOST_CFLAGS) -o $@ linux/sensor_jitter.c $(JITTER_CSV_SRC) \
		-lpthread

clean:
	rm -rf build
