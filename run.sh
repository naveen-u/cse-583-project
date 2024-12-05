#!/bin/bash

__usage="
Stackato Build & Run Helper Script.

Usage:
    $0 [<test> [<test> ...]]
    $0 -h | --help

Arguments:
    <test>          Name of test case to build.

Options:
    -h, --help      Show this help text.
"

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  echo "$__usage"
  exit 0
fi

PASS_NAME=CanaryPass
BUILD=$(pwd)/build
PASS_PATH=${BUILD}/pass/CanaryPass.so
LIB=${BUILD}/pass/rand.o
PATH_TO_TESTS=./tests
EXECUTABLES=${BUILD}/executables

mkdir -p ${BUILD}
pushd ${BUILD} > /dev/null
cmake ..
make -j8
popd > /dev/null

if [[ $(uname -m) ==  x86* ]]; then
    echo "Using RDRAND for randomness"
    clang -D __rdrand__ -mrdrnd -c $(pwd)/pass/rand.c -o ${LIB}
else
    echo "Using OpenSSL's PRNG for randomness"
    clang -c $(pwd)/pass/rand.c -o ${LIB}
fi

# If no testcase given, exit after building the pass and rand lib
if [ -z "$*" ]; then 
    exit 0
fi

mkdir -p ${EXECUTABLES}

RED='\033[0;31m'
NC='\033[0m' # No Color
echo
echo "Building tests..."
echo

for TEST in "$@"
do
    while read -r line; do echo "[$TEST] $line"; done < <({
        if [ ! -f ${PATH_TO_TESTS}/$TEST.c ]; then
            echo -e "${RED}Test not found at ${PATH_TO_TESTS}/$TEST.c${NC}!... Skipping"
            continue
        fi

        # Compile TESTmark to LLVM IR
        clang -emit-llvm -Xclang -disable-O0-optnone -o ${PATH_TO_TESTS}/$TEST.bc -c ${PATH_TO_TESTS}/$TEST.c  2>&1

        # Apply pass
        echo "Applying ${PASS_NAME}"
        opt -load-pass-plugin="${PASS_PATH}" -passes="${PASS_NAME}" ${PATH_TO_TESTS}/${TEST}.bc -o ${PATH_TO_TESTS}/${TEST}.fs.bc  2>&1

        # Generete executable for the original code
        echo "Generating binary for the original bytecode"
        clang ${PATH_TO_TESTS}/$TEST.bc -o ${EXECUTABLES}/${TEST}.original.out  2>&1

        # Generate executable for the code with pass applied
        if [[ $(uname -m) ==  x86* ]]; then
            echo "Generating binary for the bytecode with pass applied"
            clang -mrdrnd ${PATH_TO_TESTS}/${TEST}.fs.bc ${LIB} -o ${EXECUTABLES}/${TEST}.fs.out  2>&1
        else
            echo "Generating binary for the bytecode with pass applied"
            clang ${PATH_TO_TESTS}/${TEST}.fs.bc ${LIB} -o ${EXECUTABLES}/${TEST}.fs.out -lcrypto  2>&1
        fi
    })
    echo
done
