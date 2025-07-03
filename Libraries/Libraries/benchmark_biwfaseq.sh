# Benchmark
gcc benchmark.c -O3 -g -o benchmark -lm

#iterate
libraries=("biwavex" "biwfa_avx" "biwfa_og")
testcases=("10K_seq" "20K_seq" "30K_seq" "40K_seq" "50K_seq" "60K_seq" "70K_seq" "80K_seq" "90K_seq" "100K_seq")

for i in "${libraries[@]}"
do
    for x in "${testcases[@]}"
    do
        ./benchmark -l $i -t $x
    done
done