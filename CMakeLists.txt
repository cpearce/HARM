cmake_minimum_required (VERSION 3.6.1)
project (HARM)
set (CMAKE_CXX_STANDARD 11)

if (MSVC)
  add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=0)
endif()
set(gtest_force_shared_crt ON CACHE BOOL "")
add_subdirectory(googletest)

add_library(harm_main
  src/Apriori.cpp
  src/ConnectionTable.cpp
  src/CoocurrenceGraph.cpp
  src/DataStreamMining.cpp
  src/FPNode.cpp
  src/FPTree.cpp
  src/InvertedDataSetIndex.cpp
  src/Item.cpp
  src/ItemSet.cpp
  src/MinAbsSupFilter.cpp
  src/Options.cpp
  src/PatternStream.cpp
  src/StructuralStreamDriftDetector.cpp
  src/TidList.cpp
  src/VariableWindowDataSet.cpp
  src/WindowIndex.cpp
  src/utils.cpp
)

add_executable(harm src/Harm.cpp)
target_link_libraries(harm PRIVATE harm_main)

enable_testing()

# To add a gtest, add a file as tests/Test_NAME.cpp, and add NAME to
# the list of cases below.
foreach(test_case
        Apriori
        ItemSet
        InvertedDataSetIndex
        FPTree
        Streaming
        Utils)
  add_executable(Test_${test_case} tests/Test_${test_case}.cpp)
  target_include_directories(Test_${test_case} PRIVATE ${gtest_SOURCE_DIR}/include src)
  target_link_libraries(Test_${test_case} PRIVATE gtest_main harm_main)
  add_test(${test_case} Test_${test_case})
endforeach(test_case)
