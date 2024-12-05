# Stackato

> ### staccato
> [ st*uh*-**kah**-toh ] ***adjective***
>
> 1: shortened and detached when played or sung
>
> 2: characterized by performance in which the notes are abruptly disconnected
> 
> 3: composed of or characterized by abruptly disconnected elements; disjointed

An LLVM pass for thwarting and detecting Data Oriented Programming (DOP) attacks with run-time random padding and canary insertion.

## Usage
### Prerequisites
- CMake >= 3.20
- LLVM >= 18
- If system is not x86, the OpenSSL crypto library is required to generate random numbers

### Building
To build the pass and the required library, use the provided run script.
```console
$ ./run.sh
```
This generates `CanaryPass.so` and `rand.o` in the `./build/pass` directory. The pass can then be applied using `clang` or `opt`.

### Applying the Pass
#### Using `opt`
To apply the pass using `opt`:
```console
$ opt -load-pass-plugin="<path_to_CanaryPass.so>" -passes="CanaryPass" <source_file>
```
When using opt, ensure that `rand.o` is linked when creating the final binary on x86 machines, and the OpenSSL Crypto lib is linked if building on arm machines.

#### Using `clang`
To apply the pass using `clang`, use one of the below based on system architecture:
```console
$ clang -Xclang -disable-O0-optnone -fpass-plugin="<path_to_CanaryPass.so>" <path_to_rand.o> <source_file> # for x86
$ clang -Xclang -disable-O0-optnone -fpass-plugin="<path_to_CanaryPass.so>" -lcrypto <source_file> # for arm
```

### Testing
To generate executables for the test cases in `./tests/`, use the run script:
```console
$ ./run.sh foo      # This generates executables for ./tests/foo.c
```
Two executables are generated in `./build/executables`:
- `foo.original.out`: Executable for the unmodified program
- `foo.fs.out`: Executable with the pass applied

## Demo
Refer https://github.com/naveen-u/min-dop for a demonstration of Stackato.

## References
This project was based on:
- Aga, M. T., & Austin, T. (2019, February). Smokestack: Thwarting DOP attacks with runtime stack layout randomization. In _2019 IEEE/ACM International Symposium on Code Generation and Optimization (CGO)_ (pp. 26-36). IEEE.
- Hu, H., Shinde, S., Adrian, S., Chua, Z. L., Saxena, P., & Liang, Z. (2016, May). Data-oriented programming: On the expressiveness of non-control data attacks. In _2016 IEEE Symposium on Security and Privacy (SP)_ (pp. 969-986). IEEE.

## Authors
- Akash Poptani
- Archit Bhatnagar
- Erik Chi
- Naveen Unnikrishnan
- Serra Dane
