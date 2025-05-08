CC = g++
CFLAGS = -fopenmp -O2

all: run plot

lab5: lab5.cpp
	$(CC) $(CFLAGS) lab5.cpp -o lab5

run: lab5
	./lab5

plot: timing.dat plot_results.gnu
	gnuplot plot_results.gnu

clean:
	rm -f lab5 timing.dat *.png
