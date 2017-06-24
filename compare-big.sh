printf "Starting test\n\n";

function run_original {
    cd original/;
    make clean > /dev/null;
    make > /dev/null;
    time ./gol < ../input-big.in > ../original_out.txt;
    make clean > /dev/null;
    cd ..;
}

function run_distr {
    make clean > /dev/null;
    make > /dev/null;
    time mpirun -np 4 ./gol < ./input-big.in > ./distr_out.txt;
    make clean > /dev/null;
}

# run sequential
printf "Running sequential first\n";
run_original;

printf "===================\n\n";

# run distributed
printf "Running distributed (np = 4)\n";
run_distr;

printf "===================\n\n";

printf "Comparing results:\n(if no output, they are equal)\n";
cmp ./original_out.txt ./distr_out.txt;