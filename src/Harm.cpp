// Copyright 2014, Chris Pearce & Yun Sing Koh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include "utils.h"
#include "Item.h"
#include "ItemSet.h"
#include "InvertedDataSetIndex.h"
#include "AprioriFilter.h"
#include "TidList.h"
#include "debug.h"
#include "Options.h"
#include "List.h"
#include "PatternStream.h"
#include <sstream>
#include "ItemMap.h"
#include "WindowIndex.h"
#include "FPTree.h"
#include "DataStreamMining.h"
#include "VariableWindowDataSet.h"
#include "StructuralStreamDriftDetector.h"

using namespace std;

// Uncomment to run unit tests.
// TODO: Use GTest or somesuch instead.
// #define RUN_UNIT_TESTS

void Apriori(Options& options);
void Apriori_Test();
void ResultSet_Test();
void InvertedDataSetIndex_Test();

int main(int argc, const char* argv[]) {

  InitializePopulationCount();
  srand((int)time(0));

  #ifdef RUN_UNIT_TESTS
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  Item::Test();
  ItemSet::Test();
  TidList::Test();
  CoocurrenceGraph::Test();
  InvertedDataSetIndex_Test();
  Item::SetCompareMode(Item::INSERTION_ORDER_COMPARE);
  WindowIndex::Test();
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  Apriori_Test();
  Thread::Test();
  MinAbsSupFilter::Test();
  Item::ResetBaseId();
  Test_FPTree();
  Item::ResetBaseId();
  List_Test();
  PatternStream_Test();
  ItemMap_Test();
  Item::SetCompareMode(Item::INSERTION_ORDER_COMPARE);
  StructuralStreamDriftDetector::Test();
  Item::ResetBaseId();
  StructuralStreamDriftDetector::TestSpearman();
  VariableWindowDataSet::Test();

  return 0; //finish testing
  #endif

  cout << "HARM: Compiled " << __DATE__ << ":" << __TIME__ << endl;
  cout << "Copyright (c) 2011, Chris Pearce & Yun Sing Koh" << endl << endl;



  Options options;

  if (!ParseArgs(argc, argv, options)) {
    cerr << "Fail: Error parsing args...\n\n";
    PrintUsage();
    #ifdef _DEBUG
    system("pause");
    #endif
    return -1;
  }

  if (!InitLog(options)) {
    return -1;
  }
  #ifdef _DEBUG
  ShowOptions(options);
  #endif

  time_t stime;
  time(&stime);
  Log("\nStart time: %s\n", GetTimeStr(stime).c_str());

  DurationTimer timer;

  switch (options.mode) {
    case kApriori:
    case kMinAbssup:
      Apriori(options);
      break;
    case kFPTree:
    case kCanTree:
    case kCpTree:
    case kCpTreeStream:
    case kSpoTree:
    case kSpoTreeStream:
    case kStream:
    case kExtrapTreeStream:
    case kDiscTree:
    case kDDTreeStream:
      FPTreeMiner(options);
      break;
    case kSSDD:
      MineDataStream(options);
      break;
    default:
      cout << "ERROR: No mode specified\n";
  }

  Log("Processing took %.3lfs\n", timer.Seconds());

  time_t endTime;
  time(&endTime);
  Log("\nEnd time: %s\n", GetTimeStr(endTime).c_str());
  if (options.ddResultsFile.is_open()) {
    options.ddResultsFile << GetTimeStr(endTime).c_str() << endl;
    options.ddResultsFile.close();
  }
  CloseLog();

  #ifdef _DEBUG
  system("pause");
  #endif
  return 0;
}
