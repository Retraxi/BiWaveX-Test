# Benchmark
gcc benchmark.c -O3 -g -o benchmark -lm

#iterate
libraries=("biwfa_avx")
testcases=("100K_seq" "80K_seq" "50K_seq" "30K_seq" "10K_seq")

for i in "${libraries[@]}"
do
    for x in "${testcases[@]}"
    do
        ./benchmark -l $i -t $x
    done
done