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

#include "Options.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "time.h"
#include "stdio.h"
#include "utils.h"
#include "debug.h"
#include <limits.h>
#include <cfloat>
#include <thread>
#include <map>
#include <ctime>

using namespace std;

const static double DefaultMinConf = 0.9;
const static double DefaultMinLift = 1.0;
const static int DefaultBlockSize = 10000;
const static double DefaultDriftThreshold = 20.0;

struct Mode {
  const char* name;
  eRunModeType type;
};

static Mode sTypes[] = {
  {"apriori", kApriori},
  {"minabssup", kMinAbssup},
  {"fptree", kFPTree},
  {"cantree", kCanTree},
  {"cptree", kCpTree},
  {"cptreeStream", kCpTreeStream},
  {"spotree", kSpoTree},
  {"spotreeStream", kSpoTreeStream},
  {"stream", kStream},
  {"ExtrapStream", kExtrapTreeStream},
  {"disctree", kDiscTree},
  {"DDTreeStream", kDDTreeStream},
  {"SSDD", kSSDD},
  {"DBDD", kDBDD},
};

eRunModeType GetRunMode(string& mode) {
  for (unsigned i = 0; i < ARRAY_LENGTH(sTypes); ++i) {
    if (strcmp(sTypes[i].name, mode.c_str()) == 0) {
      return sTypes[i].type;
    }
  }
  return kError;
}

string GetRunMode(eRunModeType kMode) {
  for (unsigned i = 0; i < ARRAY_LENGTH(sTypes); ++i) {
    if (sTypes[i].type == kMode) {
      return sTypes[i].name;
    }
  }
  return "Error!";
}

static bool ModeRequiresBlockSize(eRunModeType aMode) {
  switch (aMode) {
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
    case kSSDD:
    case kDBDD:
      return true;
    default:
      return false;
  }
}

static bool ModeRequiresCPSortInterval(eRunModeType aMode) {
  switch (aMode) {
    case kCpTree:
    case kCpTreeStream:
    case kDDTreeStream:
      return true;
    default:
      return false;
  }
}

bool ParseCommandLine(int argc,
                      const char* argv[],
                      std::map<string, string>& aOutOptions) {
  if (argc < 1) {
    return false;
  }
  aOutOptions.clear();
  int index = 1;
  while (index < argc) {
    string name(argv[index++]);
    string value;
    if (name.find("-") == string::npos) {
      cerr << "Unnamed command line argument " << name << endl;
      return false;
    }
    if (aOutOptions.find(name) != aOutOptions.end()) {
      cerr <<  "Command line argument " << name << " can't be specified twice!" << endl;
      return false;
    }
    if (index < argc && argv[index][0] != '-') {
      // Is a parameter named by this arg.
      value = string(argv[index++]);
    }
    aOutOptions[name] = value;
  }
  return true;
}

bool ParseDouble(const string& aName,
                 map<string, string>& aArgs,
                 double& aOutValue,
                 bool aRequired,
                 double aDefault,
                 double aMin,
                 double aMax) {
  const string key = "-" + aName;
  const auto itr = aArgs.find(key);
  if (itr == aArgs.end() || itr->second.empty()) {
    if (aRequired) {
      cerr << "Fail: " << aName << " (float) not supplied with " << key << " option" << endl;
      return false;
    }
    aArgs.erase(key);
    aOutValue = aDefault;
    return true;
  }
  double val = 0;
  try {
    val = std::stod(itr->second);
  } catch (...) {
    cerr << "Fail: Unable to parse " << aName << " value of " << val << " as float!" << endl;
    return false;
  }

  if (val < aMin || val > aMax) {
    cerr << "Fail: " << aName << " must be in range [" << aMin << "," << aMax << "]" << endl;
    return false;
  }
  aOutValue = val;
  aArgs.erase(key);
  return true;
}

bool ParseDouble(const string& aName,
                 map<string, string>& aArgs,
                 double& aOutValue,
                 bool aRequired,
                 double aDefault) {
  return ParseDouble(aName,
                     aArgs,
                     aOutValue,
                     aRequired,
                     aDefault,
                     DBL_MIN,
                     DBL_MAX);
}

bool ParseInt(const string& aName,
              map<string, string>& aArgs,
              int& aOutValue,
              bool aRequired,
              int aDefault) {
  aOutValue = aDefault;
  const string key = "-" + aName;
  const auto itr = aArgs.find(key);
  if (itr == aArgs.end() || itr->second.empty()) {
    if (aRequired) {
      cerr << "Fail: " << aName << " (int) not supplied with " << key << " option" << endl;
    }
    return !aRequired;
  }
  const string& val = itr->second;
  try {
    aOutValue = std::stoi(val);
  } catch (...) {
    cerr << "Fail: Unable to parse " << aName << " value of " << val << " as integer!" << endl;
    return false;
  }
  aArgs.erase(key);
  return true;
}

static bool
ParseBoolArg(const string& aName,
             map<string, string>& aArgs) {
  const auto key = "-" + aName;
  bool rv = aArgs.find(key) != aArgs.end();
  aArgs.erase(key);
  return rv;
}

bool ParseArgs(int argc, const char* argv[], Options& options) {

  map<string, string> args;
  if (!ParseCommandLine(argc, argv, args)) {
    return false;
  }
  std::cout << "Command line args:" << endl;
  for (auto itr = args.begin();
       itr != args.end();
       itr++) {
    const string& name = itr->first;
    const string& value = itr->second;
    std::cout << "\t" << name << " = " << value << endl;
  }

  options.dataPoints = 5;
  options.discSortThreshold = 0;
  options.driftThreshold = DefaultDriftThreshold;

  // -i input file.
  if (args.count("-i") != 1) {
    cerr << "Fail: No input filename supplied with '-i' option.\n";
    return false;
  }
  options.inputFileName = args["-i"];
  args.erase("-i");

  // -o output file prefix.
  if (args.count("-o") != 1) {
    cerr << "Fail: No output file prefix supplied with '-o' option.\n";
    return false;
  }
  options.outputFilePrefix = args["-o"];
  args.erase("-o");

  // -m <mode>
  if (args.count("-m") != 1 ||
      ((options.mode = GetRunMode(args["-m"])) == kError)) {
    cerr << "Fail: Missing or invalid run mode supplied with '-m' option." << endl;
    cerr << "Valid run modes are:\n";
    for (unsigned j = 0; j < ARRAY_LENGTH(sTypes); ++j) {
      cerr << "\t" << GetRunMode(sTypes[j].type) << endl;
    }
    return false;
  }
  args.erase("-m");

  // Minconf parameter
  if (!ParseDouble("minconf", args, options.minConf, false, DefaultMinConf)) {
    return false;
  }

  // Minlift parameter
  if (!ParseDouble("minlift", args, options.minLift, false, DefaultMinLift)) {
    return false;
  }

  // Minsup, required for all modes except minabssup.
  if (options.mode == kMinAbssup) {
    if (args.find("-minsup") != args.end()) {
      cerr << "Fail: Don't need to specify a -minsup in in minabssup mode." << endl;
      return false;
    }
  }
  if (!ParseDouble("minsup", args, options.minSup, true, 0)) {
    return false;
  }

  if (ModeRequiresBlockSize(options.mode) &&
      !ParseInt("blockSize", args, options.blockSize, false, DefaultBlockSize)) {
    return false;
  }

  if (!ParseInt("n", args, options.numThreads, false, std::thread::hardware_concurrency())) {
    return false;
  }

  options.countRulesOnly = ParseBoolArg("count-rules-only", args);
  options.countItemSetsOnly = ParseBoolArg("count-itemsets-only", args);

  if (ModeRequiresCPSortInterval(options.mode) &&
      !ParseInt("cp-sort-interval", args, options.cpSortInterval, true, 0)) {
    return false;
  }

  if ((options.mode == kSpoTree || options.mode == kSpoTreeStream) &&
      !ParseDouble("spo-entropy-threshold", args, options.spoSortThreshold, true, 0)) {
    return false;
  }

  if (options.mode == kDiscTree &&
      !ParseInt("disc-sort-interval", args, options.discSortThreshold, true, 0)) {
    return false;
  }

  // Tree prune depth, required only in kDisc mode.
  if (!ParseInt("tree-prune-depth",
                args,
                options.treePruneDepth,
                (options.mode == kDiscTree),
                UINT32_MAX)) {
    return false;
  }

  if (options.mode == kDiscTree &&
      !ParseDouble("min-disc",
                   args,
                   options.min_discriminativeness,
                   false,
                   std::numeric_limits<double>::quiet_NaN())) {
    return false;
  }

  if (options.mode == kExtrapTreeStream &&
      !ParseDouble("extrap-entropy-threshold", args, options.ExtrapSortThreshold, true, 0)) {
    return false;
  }

  if (args.find("-log-tree-metrics") != args.end()) {
    const string& x = args["-log-tree-metrics"];
    vector<string> k;
    Tokenize(x, k, ",");
    for (unsigned u = 0; u < k.size(); u++) {
      unsigned j = atoi(k[u].c_str());
      options.logTreeTxn.push_back(j);
    }
    args.erase("-log-tree-metrics");
  }

  if (options.mode == kExtrapTreeStream || options.mode == kDDTreeStream) {
    options.useKernelRegression = ParseBoolArg("use-kernel-regressiony", args);

    // Number of data points to train kernel regression parameter
    if (!ParseInt("-kernel-datapoints", args, options.dataPoints, true, 0)) {
      return false;
    }
  }

  if (options.mode == kDDTreeStream) {
    if (!ParseInt("stopDepth", args, options.stopDepth, true, 0) ||
        !ParseInt("cmpDepthTh", args, options.cmpDepthThreshold, false, 1) ||
        !ParseDouble("driftThreshold", args, options.driftThreshold, false, 20.0)) {
      return false;
    }
    auto itr = args.find("-driftResultsFile");
    if (itr == args.end()) {
      cerr << "Fail: No drift results file supplied with '-driftResultsFile' option.\n";
      return false;
    }
    args.erase("-driftResultsFile");
    const string& filename = itr->second;
    options.ddResultsFile.open(filename, ios::app);
    if (!options.ddResultsFile.is_open()) {
      cerr << "Fail: Unable to open drift results file : " << filename << endl;
      return false;
    }
  }

  if (options.mode == kSSDD) {
    // Added by Minh
    //options.countItemSetsOnly = ParseBoolArg("count-itemsets-only", args);
    options.have_automatic_ssdd_structural_drift_threshold = ParseBoolArg("automatic-ssdd-structural-drift-threshold", args);
    options.adaptive_windows = ParseBoolArg("adaptive-windows", args);
    options.almost_exact = ParseBoolArg("almost-exact", args);
    options.have_ssdd_structural_drift_threshold =
      args.find("-ssdd-tree-drift-threshold") != args.end();
    if (!ParseDouble("ssdd-tree-drift-threshold",
                     args,
                     options.ssdd_structural_drift_threshold,
                     false,
                     std::numeric_limits<double>::quiet_NaN(),
                     0.0,
                     1.0)) {
      return false;
    }

    options.have_ssdd_structural_merge_threshold =
      args.find("-ssdd-tree-merge-threshold") != args.end();
    if (!ParseDouble("ssdd-tree-merge-threshold",
                     args,
                     options.ssdd_structural_merge_threshold,
                     false,
                     std::numeric_limits<double>::quiet_NaN(),
                     0.0,
                     1.0)) {
      return false;
    }

    options.have_ssdd_item_frequency_drift_threshold =
      args.find("-ssdd-item-freq-drift-threshold") != args.end();
    if (!ParseDouble("ssdd-item-freq-drift-threshold",
                     args,
                     options.ssdd_item_frequency_drift_threshold,
                     false,
                     std::numeric_limits<double>::quiet_NaN(),
                     0.0,
                     1.0)) {
      return false;
    }

    options.have_ssdd_item_frequency_merge_threshold
      = args.find("-ssdd-item-freq-merge-threshold") != args.end();
    if (!ParseDouble("ssdd-item-freq-merge-threshold",
                     args,
                     options.ssdd_item_frequency_merge_threshold,
                     false,
                     std::numeric_limits<double>::quiet_NaN(),
                     0.0,
                     1.0)) {
      return false;
    }

    options.ssdd_window_cmp = ParseBoolArg("ssdd-window-cmp", args);
    options.dd_print_blocks = ParseBoolArg("ssdd-print-blocks", args);

    if ((!options.have_automatic_ssdd_structural_drift_threshold && !options.have_ssdd_structural_drift_threshold) &&
        !options.have_ssdd_item_frequency_drift_threshold) {
      cerr << "Fail: with SSDD mode you must specify either of "
           << "-automatic-ssdd-tree-drift-threshold or -ssdd-tree-drift-threshold or -ssdd-item-freq-drift-threshold" << endl;
      //modified by Minh
      return false;
    }
  }

  if (options.mode == kDBDD) {
    options.dd_print_blocks = ParseBoolArg("dbdd-print-blocks", args);
    if (!ParseDouble("dbdd-delta",
                     args,
                     options.dbdd_delta,
                     true,
                     std::numeric_limits<double>::quiet_NaN(),
                     0.0,
                     1.0)) {
      cerr << "Fail: with DBDD mode you must specify -dbdd-delta in range [0,1]" << endl;
      return false;
    }
    options.adaptive_windows = true;
  }

  if (args.size()) {
    cerr << "Fail: extraneous or unrecognised parameters:" << endl;
    auto itr = args.begin();
    while (itr != args.end()) {
      const string& key = itr->first;
      const string& value = itr->second;
      cerr << "\t" << key << " = " << value << endl;
      itr++;
    }
    return false;
  }

  time(&options.startTime);

  return true;
}

void PrintUsage() {
  cout << "HARM - Association Rule Mining\n\n";

  cout << "Parameters:\n\n";

  cout << "-i <input CSV file name> (*)\n";
  cout << "-m <mode> ; one of {";
  for (unsigned j = 0; j < ARRAY_LENGTH(sTypes); ++j) {
    cout << GetRunMode(sTypes[j].type) << ((j + 1 != ARRAY_LENGTH(sTypes)) ? ", " : "}\n");
  }
  cout << "-o <output prefix> ; prepended to output files, can include a directory path. (*)\n";
  cout << "-minconf <c> ; sets minconf for rule pruning to c, optional, default=0.9.\n";
  cout << "-minlift <l> ; sets minlift for rule pruning to l, optional, default=1.0.\n";
  cout << "-minsup <s> ; sets minimum support. Required for apriori or tree based mining.\n";
  cout << "-automatic-ssdd-structural-drift-threshold ; sets automatic ssdd structural drift threshold. Required for SSDD mining.\n";
  cout << "-blockSize <s> ; sets blockSize. Required for stream based mining. (default 10000).\n";
  cout << "-n <threads> ; sets number of threads. Default=autodetect.\n";
  cout << "-count-rules-only ; only counts the rules, doesn't write them to disk.\n";
  cout << "-count-itemsets-only ; doesn't write itemsets or rules to disk, just counts itemsets.\n";
  cout << "-cp-sort-interval <n> ; number of transactions between resorting tree in cptree mode.\n";
  cout << "-disc-sort-interval <n> ; number of transactions between resorting tree in disctree mode.\n";
  cout << "-log-tree-metrics=n1,n2,n,,, ; log tree size on transaction n1, n2, etc.\n";
  cout << "-tree-prune-depth <d> ; sets the depth below which fptree nodes are pruned.\n";
  cout << "-min-disc <d> ; sets the minimum discriminativeness, items with d-values less than this are pruned.\n";
  cout << "-stopDepth <d> ; sets the stop depth for DDTree, default is 0 (not using stop depth).\n";
  //  cout << "-use-kernel-regression ; uses kernel regression as the method of extrapolation in ExtrapTree. The default is linear extrapolation.\n";
  cout << "Paremeters marked with (*) are required.\n\n";

  cout << "Output is stored in the following files:\n";
  cout << GetOutputLogFileName(string("<output prefix>"))
       << " - stores stats about run, etc\n";
  cout << GetOutputItemsetsFileName(string("<output prefix>"))
       << " - itemsets produced by run, with stats.\n";
  cout << GetOutputRuleFileName(string("<output prefix>"))
       << " - rules produced by run, with stats.\n\n";
}

string GetTimeStr(time_t& t) {
  return string(asctime(localtime(&t)));
}

void ShowOptions(Options& options) {
  Log("HARM - Association Rule Mining\n\n");
  Log("Parameters:\n");
  Log("Input file: %s\n", options.inputFileName.c_str());
  Log("Output log file: %s\n", GetOutputLogFileName(options.outputFilePrefix).c_str());
  Log("Output itemsets File: %s\n", GetOutputItemsetsFileName(options.outputFilePrefix).c_str());
  Log("Output rules file: %s\n", GetOutputRuleFileName(options.outputFilePrefix).c_str());
  Log("Start time: %s\n", GetTimeStr(options.startTime).c_str());
  Log("Mode: %s\n", GetRunMode(options.mode).c_str());
  if (options.mode == kApriori || options.mode == kFPTree) {
    Log("Minsup: %lf\n", options.minSup);
  }
  Log("MinConf: %lf\n", options.minConf);
  Log("MinLift: %lf\n", options.minLift);
  //  if (options.mode == kExtrapTreeStream)
  //  Log("ExtrapMethod: &d", options.useKernelRegression);
  Log("Num threads: %u\n", options.numThreads);
}
