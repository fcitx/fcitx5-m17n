# NOTE: This is required as testmim.cc uses C++11 features
CXX := clang++
# For "step-in" output of testmim
CXXFLAGS := $(CXXFLAGS) -DSTEP
