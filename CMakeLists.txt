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
  src/AprioriFilter.h
  src/CPTreeFunctor.h
  src/CanTreeFunctor.h
  src/ConnectionTable.cpp
  src/ConnectionTable.h
  src/CoocurrenceGraph.cpp
  src/CoocurrenceGraph.h
  src/DDTreeFunctor.h
  src/DataSetReader.h
  src/DataStreamMining.cpp
  src/DataStreamMining.h
  src/DiscTreeFunctor.h
  src/ExtrapTreeFunctor.h
  src/FPNode.cpp
  src/FPNode.h
  src/FPTree.cpp
  src/FPTree.h
  src/InvertedDataSetIndex.cpp
  src/InvertedDataSetIndex.h
  src/Item.cpp
  src/Item.h
  src/ItemMap.h
  src/ItemSet.cpp
  src/ItemSet.h
  src/List.h
  src/MinAbsSupFilter.cpp
  src/Options.cpp
  src/Options.h
  src/PatternStream.cpp
  src/PatternStream.h
  src/SpoTreeFunctor.h
  src/StructuralStreamDriftDetector.cpp
  src/StructuralStreamDriftDetector.h
  src/TestDataSets.h
  src/TidList.cpp
  src/TidList.h
  src/VariableWindowDataSet.cpp
  src/VariableWindowDataSet.h
  src/WindowIndex.cpp
  src/WindowIndex.h  
  src/debug.h
  src/utils.cpp
  src/utils.h
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
