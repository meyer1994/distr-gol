CC=mpicc

FLAGS=-O3 -Wno-unused-result
DEBUG=-DDEBUG
RESULT=-DRESULT

gol: gol.c
	$(CC) $(RESULT) $(FLAGS) gol.c -o gol

debug:
	$(CC) $(RESULT) $(DEBUG) $(FLAGS) gol.c -o gol

run:
	mpirun -np 3 ./gol < input-little.in

clean:
	rm -rf gol