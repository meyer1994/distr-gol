CC=mpicc

FLAGS=-O2 -Wno-unused-result
DEBUG=-DDEBUG
RESULT=-DRESULT

gol: gol.c
	$(CC) $(FLAGS) gol.c -o gol

result:
	$(CC) $(RESULT) $(FLAGS) gol.c -o gol

debug:
	$(CC) $(RESULT) $(DEBUG) $(FLAGS) gol.c -o gol

small:
	time mpirun -np 4 ./gol < input-little.in

big:
	time mpirun -np 4 ./gol < input-big.in

clean:
	rm -rf gol