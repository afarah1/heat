CC=gcc
MPCC=mpicc
STD=-std=c99
WARN=-Wall -Wextra -Wpedantic -Wformat-security -Wfloat-equal -Wshadow\
     -Wconversion -Winline #-Wpadded
OPT=-O2 -march=native -ffinite-math-only -fno-signed-zeros -DLOG_LEVEL=LOG_LEVEL_WARNING
DBG=-O0 -g -ggdb -DLOG_LEVEL=LOG_LEVEL_DEBUG
EXTRA=-I. -I../logging
SDL=$(shell pkg-config sdl2 --cflags --libs)
LINK=$(SDL)
FLAGS=$(STD) $(WARN) $(OPT) $(EXTRA) $(LINK)

all: par display

par:
	$(MPCC) par.c -o heat $(FLAGS)

seq:
	$(CC) seq.c -o heat $(FLAGS)

display:
	$(CC) display.c graphics_sdl.c -o display $(FLAGS)

clean:
	rm -f heat display
