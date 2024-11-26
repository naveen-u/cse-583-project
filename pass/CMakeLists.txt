add_llvm_pass_plugin(CanaryPass canarypass.cpp rand.c)
# SET(CMAKE_C_FLAGS "-mrdrnd")