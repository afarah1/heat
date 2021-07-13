CC=gcc
STD=-std=c99
WARN=-Wall -Wextra -Wpedantic -Wformat-security -Wfloat-equal -Wshadow\
     -Wconversion -Winline #-Wpadded
OPT=-O2 -march=native -ffinite-math-only -fno-signed-zeros -DLOG_LEVEL=LOG_LEVEL_WARNING
DBG=-O0 -g -ggdb -DLOG_LEVEL=LOG_LEVEL_DEBUG
EXTRA=-I. -I../logging -fopenmp
LINK=
FLAGS=$(STD) $(WARN) $(OPT) $(EXTRA) $(LINK)

all: heat

heat:
	$(CC) heat.c -o heat $(FLAGS)

clean:
	rm -f heat
