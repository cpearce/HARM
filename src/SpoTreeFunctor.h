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

#include "FPTree.h"
#include "FPNode.h"

#include <vector>

class SpoRanks {
public:
  SpoRanks(FPNode* root)
    : diffSum(0)
    , fptree(root)
  {
  }

  // Called after a txn has been inserted. Therefore every item in the
  // transaction must have had its count incremented by 1. So ensure that
  // the ranks for each item in the transaction are still correct.
  void Adjust(const std::vector<Item>& txn) {
    for (unsigned i = 0; i < txn.size(); ++i) {
      Item item = txn[i];
      if (!lookup.Contains(item)) {
        unsigned index = unsigned(ranking.size());
        ranking.push_back(item);
        ASSERT(ranking[index] == item);
        lookup.Set(item, index);
        storedRank.Set(item, index);
      }
      unsigned r = lookup.Get(item);
      while (r > 0) {
        Item prev = ranking[r - 1];
        unsigned count = fptree->freq->Get(item, 0);
        unsigned prevCount = fptree->freq->Get(prev, 0);
        if (prevCount > count) {
          break;
        }
        if (prevCount == count && prev.GetId() < item.GetId()) {
          break;
        }
        // The item belongs before the previous item. Swap them.
        ASSERT(diffSum - delta(item) - delta(prev) >= 0);
        diffSum -= delta(item);
        diffSum -= delta(prev);

        ranking[r - 1] = item;
        ranking[r] = prev;

        lookup.Set(prev, r);
        lookup.Set(item, r - 1);

        diffSum += delta(item);
        diffSum += delta(prev);

        #ifdef _DEBUG
        VerifyDiffSumValid();
        #endif
        r--;
      }
    }
  }

  void ResetEntropy() {
    for (unsigned i = 0; i < ranking.size(); i++) {
      Item item = ranking[i];
      ASSERT(lookup.Get(item) == i);
      storedRank.Set(item, i);
    }
    diffSum = 0;
    ASSERT(GetRankEntropy() == 0);
  }

  double GetRankEntropy() const {
    double n = double(ranking.size());
    return diffSum / (n * (n - 1) + floor(n * n / 4));
  }

private:

  void VerifyDiffSumValid() {
    unsigned sum = 0;
    for (unsigned i = 0; i < ranking.size(); ++i) {
      sum += delta(ranking[i]);
    }
    ASSERT(sum == diffSum);
  }

  unsigned delta(Item item) {
    unsigned rank = lookup.Get(item);
    unsigned stored = storedRank.Get(item);
    return rank > stored ? (rank - stored) : (stored - rank);
  }

  unsigned diffSum;

  std::vector<Item> ranking;

  // Maps an item to its index in the ranking vector. This is also the
  // items rank.
  ItemMap<unsigned> lookup;

  // Maps an item to its stored rank. The stored rank is the rank an item
  // had at the last sort.
  ItemMap<unsigned> storedRank;

  FPNode* fptree;
};

class SpoTreeFunctor : public FPTreeFunctor {
public:

  SpoTreeFunctor(FPNode* aTree,
                 double _threshold,
                 int aBlockSize,
                 bool aIsStreaming,
                 const std::vector<unsigned>& aTxnNums,
                 DataSet* aIndex,
                 Options& aOptions)
    : FPTreeFunctor(aTree, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions),
      threshold(_threshold),
      ranks(aTree) {
  }

  void OnLoad(const std::vector<Item>& txn) {
    std::vector<Item> t(txn);
    mTree->SortTransaction(t);
    mTree->Insert(t);
    ranks.Adjust(t);

    double entropy = ranks.GetRankEntropy();
    if (entropy > threshold) {
      Log("Sorting Spotree at transaction=%u, entropy=%lf\n", mTxnNum, entropy);
      unsigned before = mTree->NumNodes();
      mTree->Sort();
      unsigned after = mTree->NumNodes();
      Log("Before sort Spotree had %u nodes, after sort %u\n", before, after);
      ranks.ResetEntropy();
    }

    FPTreeFunctor::OnLoad(txn);
  }

  void OnUnload(const std::vector<Item>& txn) {
    ASSERT(mIsStreaming); // Should only be called in streaming mode.
    std::vector<Item> t(txn);
    if (mTree->iList) {
      sort(t.begin(), t.end(),  ItemMapCmp<unsigned>(*mTree->iList));
    } else {
      sort(t.begin(), t.end(), AppearanceCmp());
    }
    mTree->Remove(t);
  }

  void OnEndLoad() override {
    // Sort the tree at the end of load, if we're going to mine after
    // loading the dataset (if we're not streaming).
    if (!mIsStreaming) {
      mTree->Sort();
      ASSERT(mTree->IsSorted());
    }
  }

  double threshold;
  SpoRanks ranks;
};