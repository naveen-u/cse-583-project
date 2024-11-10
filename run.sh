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

# First Compile to Bytecode
clang -emit-llvm -Xclang -disable-O0-optnone -o ${PATH_TO_BENCHMARK}/$BENCH.bc -c ${PATH_TO_BENCHMARK}/$BENCH.c

# Apply Optimization
# opt -o ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc -load ${PATH_MYPASS} ${NAME_MYPASS} < ${PATH_TO_BENCHMARK}/${1}.bc
echo "Applying ${NAME_MYPASS}"
opt -load-pass-plugin="${PATH_MYPASS}" -passes="${NAME_MYPASS}" ${PATH_TO_BENCHMARK}/${BENCH}.bc -o ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc

# # Generete Binary for the original bytecode
echo "Generating binary for the original bytecode"
clang ${PATH_TO_BENCHMARK}/$BENCH.bc -o ${EXECUTABLES}/${BENCH}.original.o

# clang -c $(pwd)/shadowClone/randGen.c -o ${LIB}

clang -mrdrnd -c $(pwd)/pass/rand.c -o ${LIB}

# # Generete Binary for the optimized bytecode
echo "Generating binary for the optimized bytecode"
clang -g -mrdrnd ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc ${LIB} -o ${EXECUTABLES}/${BENCH}.fs.o


# cc -c Shadowclone/randGen.c -o ${LIB}
# clang -Xclang -load -Xclang ${PATH_MYPASS} -c -fPIC ${PATH_TO_BENCHMARK}/${BENCH}.c -o ${PATH_TO_BENCHMARK}/${BENCH}.o
# cc ${LIB} ${PATH_TO_BENCHMARK}/${BENCH}.o
# ./a.out