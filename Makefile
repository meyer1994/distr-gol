CC=mpicc

FLAGS=-O3 -Wno-unused-result
DEBUG=-DDEBUG
RESULT=-DRESULT

gol: gol.c
	$(CC) $(RESULT) $(FLAGS) gol.c -o gol

clean:
	rm -rf gol