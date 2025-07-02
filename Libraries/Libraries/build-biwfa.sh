# Core Implementation
cd ./Bi-WaveX
make clean all
cd wavefront && nasm -f elf64 -g -F dwarf -o avx512_wavefront_next_iter.o avx512_wavefront_next_iter.asm && nasm -f elf64 -g -F dwarf -o avx_wavefront_extension_iteration.o avx_wavefront_extension_iteration.asm && nasm -f elf64 -g -F dwarf -o avx_backtrace_matches_iter.o avx_backtrace_matches_iter.asm && nasm -f elf64 -g -F dwarf -o avx512_wavefront_overlap_breakpoint_check.o avx512_wavefront_overlap_breakpoint_check.asm
cd ..
cd examples && gcc -g -fPIE -pie wfa_basic.c ../wavefront/avx_wavefront_extension_iteration.o ../wavefront/avx512_wavefront_next_iter.o ../wavefront/avx_backtrace_matches_iter.o ../wavefront/avx512_wavefront_overlap_breakpoint_check.o -O3 -o wfa_basic -I.. -L../lib -lwfa -lm -march=skylake-avx512
cd ../../
ls
# Intrinsics Library
cd ./BiWFA_avx
make clean all
cd examples && gcc -g -fPIE -pie wfa_basic.c -O3 -o wfa_basic -I.. -L../lib -lwfa -lm -march=skylake-avx512
cd ../../
ls
# AVXless Library
cd ./BIWFA_og
make clean all
cd examples && gcc -g -fPIE -pie wfa_basic.c -O3 -o wfa_basic -I.. -L../lib -lwfa -lm -march=skylake-avx512
cd../../


