CFLAGS=-Wall -DTEST

all: sincos

test: all
	./sincos 493.883 > sinetable
	gnuplot -p -e "plot './sinetable' using 1:2 title 'sine', './sinetable' using 1:3 title 'cosine';"

clean:
	rm sincos sinetable

.PHONY: clean all
