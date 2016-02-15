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

#include "DataStreamMining.h"
#include "Options.h"
#include "DataSetReader.h"
#include "StructuralStreamDriftDetector.h"
#include "FPNode.h"
#include "FPTree.h"

#include <assert.h>
#include <iostream>
#include <memory>

using namespace std;

MiningContext::MiningContext(const Options& _options)
  : options(_options),
    mining_run(0) {
}

void MiningContext::Mine(FPTree* root, DataSet* dataset) {
  Log("Mining rules\n");
  mining_run++;
  string itemSetsOuputFilename =
    GetOutputItemsetsFileName(options.outputFilePrefix, mining_run);
  string rulesOutputFilename =
    GetOutputRuleFileName(options.outputFilePrefix, mining_run);
  MineFPTree(root,
             options.minSup,
             itemSetsOuputFilename,
             rulesOutputFilename,
             dataset,
             options.treePruneDepth,
             options.countItemSetsOnly,
             options.countRulesOnly,
             nullptr);
}


StreamMiner* CreateStreamMiner(const Options& options) {
  // TODO: Add other modes in here.
  assert(options.mode == kSSDD || options.mode == kDBDD);
  return new StructuralStreamDriftDetector(options.blockSize,
         options.have_automatic_ssdd_structural_drift_threshold,
         options.have_ssdd_structural_drift_threshold,
         options.ssdd_structural_drift_threshold,
         options.have_ssdd_structural_merge_threshold,
         options.ssdd_structural_merge_threshold,
         options.have_ssdd_item_frequency_drift_threshold,
         options.ssdd_item_frequency_drift_threshold,
         options.have_ssdd_item_frequency_merge_threshold,
         options.ssdd_item_frequency_merge_threshold,
         options.ssdd_window_cmp,
         options.dd_print_blocks,
         options.adaptive_windows,
         options.almost_exact,
         (options.mode == kDBDD),
         options.dbdd_delta);
}

void MineDataStream(const Options& options) {
  // Currently only SSDD mode supported in MineDataStream.
  assert(options.mode == kSSDD || options.mode == kDBDD);

  MiningContext context(options);
  unique_ptr<StreamMiner> miner(CreateStreamMiner(options));
  miner->Init(&context);

  DataSetReader reader;
  if (!reader.Open(options.inputFileName)) {
    cerr << "ERROR: Can't open " << options.inputFileName << " failing!" << endl;
    exit(-1);
  }

  TransactionId tid = 0;
  while (true) {
    Transaction transaction(tid);
    if (reader.GetNext(transaction.items)) {
      // Read a transaction, send it to the StreamMiner.
      miner->Add(transaction);
      // Increment the transaction id, so that the next transaction
      // has a monotonically increasing id.
      tid++;
    } else {
      // Failed to read, no more transactions.
      break;
    }
  }
}
