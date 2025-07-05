# Benchmark
gcc benchmark.c -O3 -g -o benchmark -lm

#iterate
libraries=("biwavex" "biwfa_og")
testcases=("80K_seq" "250bp" "250bp_seq")

for i in "${libraries[@]}"
do
    for x in "${testcases[@]}"
    do
        ./benchmark -l $i -t $x
    done
done