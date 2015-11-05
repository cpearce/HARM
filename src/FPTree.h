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

#pragma once

#include <vector>

#include "InvertedDataSetIndex.h" // For LoadFunctor
class FPNode;

// Filter that specifies whether an item should be used in a mining phase.
// This is passed into FPGrowth for some FPTree variants.
class ItemFilter {
public:
  virtual bool ShouldKeep(Item item) {
    return true;
  }
};

// Logs the number of nodes in the tree at the transaction numbers listed in
// _txnNums.
class TreeMetricsLogger {
public:
  TreeMetricsLogger(FPNode* _tree, const std::vector<unsigned>& _txnNums);

  void OnTxn();

private:
  FPNode* tree;
  const std::vector<unsigned>& txnNums;
  unsigned count;
  unsigned index;
};

// An FPTreeFunctor is a LoadFunction which has its OnLoad() function called
// whenever a transaction is loaded into the an FPTree. Variants of FPTree are
// implemented by creating a new FPTreeFunctor that affects the desired
// behaviour.
//
// Classes that inherit from this *must* call FPTreeFunctor::OnLoad() at the
// end of their OnLoad() override.
class FPTreeFunctor : public LoadFunctor {
public:
  FPTreeFunctor(FPNode* aTree,
                const std::vector<unsigned>& aTxnNums,
                int aBlockSize,
                bool aIsStreaming,
                DataSet* aIndex,
                Options& aOptions);

  void OnStartLoad() override;

  void OnLoad(const std::vector<Item>& txn) override;

  void OnUnload(const std::vector<Item>& txn) override;

  void OnEndLoad() override;

  virtual ItemFilter* GetItemFilter() {
    return nullptr;
  }

  FPNode* mTree;
  TreeMetricsLogger mLogger;
  bool mIsStreaming;
  Options& mOptions;
  unsigned mTxnNum;
  unsigned mBlockSize;
  unsigned mMiningRun;
  DataSet* mIndex;
};

void MineFPTree(FPNode* fptree,
                double minSup,
                const std::string& itemSetsOuputFilename,
                const std::string& rulesOuputFilename,
                DataSet* index,
                uint32_t treePruneDepth,
                bool countItemSetsOnly,
                bool countRulesOnly,
                ItemFilter* filter = nullptr);

void FPTreeMiner(Options& options);
void Test_FPTree();

// Item comparator that orders by the items' ids. This has the effect of
// sorting by the order in which items appeared in the data set.
// Use this with std::sort.
struct AppearanceCmp {
  bool operator()(Item a, Item b) {
    return a.GetId() < b.GetId();
  }
};

bool VerifySortedByAppearance(std::vector<Item>& txn);
