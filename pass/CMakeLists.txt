add_llvm_pass_plugin(CanaryPass canarypass.cpp allocapass.cpp rand.c)
# SET(CMAKE_C_FLAGS "-mrdrnd")