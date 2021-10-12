.PHONY: all clean
CC = mpicc

all: clean mpi_benchmark

mpi_benchmark:
	${CC} -Wall mpi_benchmark.c -o mpi_benchmark

clean:
	rm -f mpi_benchmark