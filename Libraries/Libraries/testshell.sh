# loop
testcases=("10K" "20K" "30K" "40K" "50K" "60K" "70K" "80K" "90K" "100K")
libraries=("biwavex" "biwfa_avx" "biwfa_og" "ksw2" "nw")

for i in "${libraries[@]}"
do
    for x in "${testcases[@]}"
    do
        echo "Library [$i] on Case [$x]..."
    done
done