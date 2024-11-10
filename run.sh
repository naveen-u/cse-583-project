#!/bin/bash

PATH_TO_BENCHMARK=./benchmarks
NAME_MYPASS=canarypass
PATH_MYPASS=$(pwd)/build/pass/CanaryPass.so
BENCH=$1
EXECUTABLES=$(pwd)/build/executables
LIB=$(pwd)/build/pass/rand.o

pushd ./build
cmake ..
make -j8
popd

# Compile benchmark to LLVM IR
clang -emit-llvm -Xclang -disable-O0-optnone -o ${PATH_TO_BENCHMARK}/$BENCH.bc -c ${PATH_TO_BENCHMARK}/$BENCH.c

# Apply pass
echo "Applying ${NAME_MYPASS}"
opt -load-pass-plugin="${PATH_MYPASS}" -passes="${NAME_MYPASS}" ${PATH_TO_BENCHMARK}/${BENCH}.bc -o ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc
# opt -passes=debugify ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc -o ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc

# Generete executable for the original code
echo "Generating binary for the original bytecode"
clang ${PATH_TO_BENCHMARK}/$BENCH.bc -o ${EXECUTABLES}/${BENCH}.original.out

clang -mrdrnd -c $(pwd)/pass/rand.c -o ${LIB}

# Generete executable for the code with pass applied
echo "Generating binary for the optimized bytecode"
clang -g -mrdrnd ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc ${LIB} -o ${EXECUTABLES}/${BENCH}.fs.out
