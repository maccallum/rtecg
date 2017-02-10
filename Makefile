CC=clang
O=-O3

all: O = -O3
all: rtecg_filter.o rtecg_peak.o rtecg_process.o rtecg_pantompkins.o

%.o: %.c %.h
	$(CC) $(O) -std=c99 -Wall -c $(CFLAGS) $< -o $@

test: rtecg_test.c rtecg_filter.o rtecg_peak.o rtecg_pantompkins.o testdat.h
	$(CC) -std=c99 -Wall $(CFLAGS) $(O) -c rtecg_test.c
	$(CC) -o test rtecg_test.o rtecg_filter.o rtecg_peak.o rtecg_process.o rtecg_pantompkins.o

.PHONY: clean
clean:
	rm -rf test *.o

