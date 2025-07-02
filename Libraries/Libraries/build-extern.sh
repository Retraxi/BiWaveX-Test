# Edlib
cd ./edlib
make clean all
cd ./apps/aligner && g++ -I../../edlib/include/ -std=c++14 -O3 -o aligner aligner.cpp ../../edlib/src/edlib.cpp
cd ../../../
# KSW2
cd ./ksw2
make clean all
gcc -g aligner.c ksw2_extz2_sse.o kalloc.o -O3 -o aligner -I. -march=native
cd ..
ls
# Minimap2
cd ./Minimap2
make clean all