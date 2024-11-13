#!/bin/bash

NAME_MYPASS=canarypass
PATH_TO_BENCHMARK=./benchmarks
BUILD=$(pwd)/build
PATH_MYPASS=${BUILD}/pass/CanaryPass.so
BENCH=$1
EXECUTABLES=${BUILD}/executables
LIB=${BUILD}/pass/rand.o

mkdir -p ${EXECUTABLES}
pushd ${BUILD}
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

if [[ "$(uname -m)" ==  "x86_64" ]]; then
    echo "Using RDRAND for randomness"
    clang -D __rdrand__ -mrdrnd -c $(pwd)/pass/rand.c -o ${LIB}

    echo "Generating binary for the optimized bytecode"
    clang -mrdrnd ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc ${LIB} -o ${EXECUTABLES}/${BENCH}.fs.out
else
    echo "Using OpenSSL's PRNG for randomness"
    clang -c $(pwd)/pass/rand.c -o ${LIB}

    echo "Generating binary for the optimized bytecode"
    clang ${PATH_TO_BENCHMARK}/${BENCH}.fs.bc ${LIB} -o ${EXECUTABLES}/${BENCH}.fs.out -lcrypto
fi
