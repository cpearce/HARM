# HARM - Streaming Association Rule Miner
Licensed under a Apache 2 style license.

HARM is a helpful association rule miner. It finds associations in transaction data
sets using multiple methods. 

HARM is written in C++. To build:
1. Download and install [CMake](https://cmake.org/).
2. git clone https://github.com/cpearce/HARM.git
3. git submodule init
4. git submodule update
5. cmake .

Build systems appropriate for your platform will then be generated.

Tests are gtests. To add a new test, create a new gtest file as
tests/Test_NAME.cpp, and add NAME to the test_case loop in CMakeLists.txt.
