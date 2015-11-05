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

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <string>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <fstream>
#include <time.h>

enum eRunModeType {
  kError,
  kApriori,
  kMinAbssup,
  kFPTree,
  kCanTree,
  kCpTree,
  kCpTreeStream,
  kSpoTree,
  kSpoTreeStream,
  kStream,
  kExtrapTreeStream,
  kDiscTree,
  kDDTreeStream,
  kSSDD, // Structural Stream Drift Detector
};

std::string GetRunMode(eRunModeType kMode);

class Options {
public:

  Options() {}

  // Constructor used in debug tests only.
  Options(double aMinSup,
          eRunModeType aMode,
          unsigned aCpSortInterval,
          unsigned aPanels,
          double aSpoSortThreshold,
          double aExtrapSortThreshold,
          double aDecay,
          double aNumItems,
          int aBlockSize)
    : mode(aMode),
      minSup(aMinSup),
      cpSortInterval(aCpSortInterval),
      spoSortThreshold(aSpoSortThreshold),
      ExtrapSortThreshold(aExtrapSortThreshold),
      blockSize(aBlockSize),
      min_discriminativeness(std::numeric_limits<double>::quiet_NaN()),
      have_ssdd_structural_drift_threshold(false),
      ssdd_structural_drift_threshold(0),
      have_automatic_ssdd_structural_drift_threshold(false),
      adaptive_windows(false),
      almost_exact(false),
      have_ssdd_structural_merge_threshold(false),
      ssdd_structural_merge_threshold(0),
      have_ssdd_item_frequency_drift_threshold(false),
      ssdd_item_frequency_drift_threshold(0),
      have_ssdd_item_frequency_merge_threshold(false),
      ssdd_item_frequency_merge_threshold(0),
      ssdd_window_cmp(false) {
  }

  std::string inputFileName;
  std::string outputFilePrefix;
  eRunModeType mode;
  double minConf;
  double minLift;
  double minSup;
  int32_t numThreads;
  time_t startTime;
  bool countRulesOnly;
  bool countItemSetsOnly;
  int32_t cpSortInterval;
  double spoSortThreshold;
  double ExtrapSortThreshold;
  int32_t blockSize;

  int32_t treePruneDepth;
  int32_t discSortThreshold;

  double min_discriminativeness;

  bool have_ssdd_structural_drift_threshold;
  double ssdd_structural_drift_threshold;

  //added by Minh
  bool have_automatic_ssdd_structural_drift_threshold;
  bool adaptive_windows;
  bool almost_exact;

  bool have_ssdd_structural_merge_threshold;
  double ssdd_structural_merge_threshold;

  bool have_ssdd_item_frequency_drift_threshold;
  double ssdd_item_frequency_drift_threshold;

  bool have_ssdd_item_frequency_merge_threshold;
  double ssdd_item_frequency_merge_threshold;

  bool ssdd_window_cmp;
  bool ssdd_print_blocks;

  bool useKernelRegression;
  int32_t dataPoints;
  //for drift detection
  int stopDepth;
  int cmpDepthThreshold;
  double driftThreshold;
  std::ofstream ddResultsFile;
  // Array of transaction numbers to dump tree info on.
  std::vector<unsigned> logTreeTxn;
};


bool ParseArgs(int argc, const char* argv[], Options& options);
void PrintUsage();
void ShowOptions(Options& options);

inline std::string GetOutputLogFileName(std::string prefix) {
  return prefix + ".log.txt";
}

inline std::string GetOutputItemsetsFileName(std::string prefix) {
  return prefix + ".itemsets.support.csv";
}

inline std::string GetOutputItemsetsFileName(std::string prefix, int i) {
  return prefix + ".itemsets.support-" + std::to_string(i) + ".csv";
}

inline std::string GetOutputRuleFileName(std::string prefix) {
  return prefix + ".rules.conf.lift.support.csv";
}

inline std::string GetOutputRuleFileName(std::string prefix, int i) {
  return prefix + ".rules.conf.lift.support-" + std::to_string(i) + ".csv";
}

std::string GetTimeStr(time_t& t);

bool InitLog(Options& options);
void CloseLog();

#endif
