CC = gcc
CFLAGS = -Wall -Wno-incompatible-pointer-types -Wno-implicit-int -mavx2 -mbmi2 -O2 -ffast-math -fno-math-errno -fno-tree-loop-distribute-patterns -Ijpeg-9e
LDFLAGS = -lm -l:libjpeg.a -lpng -lwebp -lwebpdemux -lvpx

PROC := $(shell grep -om1 Intel /proc/cpuinfo)
ifeq ($(PROC),Intel)
  TUNE = -mtune=intel
else
  TUNE = -mtune=native
endif

.SILENT: main clean
.PHONY: clean all

all: clean main

main: main.c
	$(CC) $(CFLAGS) $(TUNE) $^ $(LDFLAGS) -o a.out
	rm -f *.o
	echo "a.out"

clean:
	rm -f a.out *.o

