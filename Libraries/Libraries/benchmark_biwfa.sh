# Benchmark
gcc benchmark.c -O3 -g -o benchmark -lm

#iterate
libraries=("biwavex" "biwfa_avx" "biwfa_og" "ksw2")
testcases=("10K" "20K" "30K" "40K" "50K" "60K" "70K" "80K" "90K" "100K")

for i in "${libraries[@]}"
do
    for x in "${testcases[@]}"
    do
        ./benchmark -l $i -t $x
    done
done