CC=mpicc

FLAGS=-O3 -Wno-unused-result
DEBUG=-DDEBUG
RESULT=-DRESULT

gol: gol.c
	$(CC) $(RESULT) $(FLAGS) gol.c -o gol

run:
	$(CC) $(RESULT) $(FLAGS) gol.c -o gol && ./gol

clean:
	rm -rf gol