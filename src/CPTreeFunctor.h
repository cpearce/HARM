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

class CpTreeFunctor : public FPTreeFunctor {
public:
  CpTreeFunctor(FPNode* aTree,
                unsigned _interval,
                int aBlockSize,
                bool aIsStreaming,
                const std::vector<unsigned>& aTxnNums,
                DataSet* aIndex,
                Options& aOptions)
    : FPTreeFunctor(aTree, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions),
      interval(_interval),
      count(0) {
  }


  void OnLoad(const std::vector<Item>& txn) {
    std::vector<Item> t(txn);
    mTree->SortTransaction(t);
    mTree->Insert(t);

    count++;
    if (count == interval) {
      Log("Sorting CPTree at transaction %u\n", mTxnNum);
      unsigned before = mTree->NumNodes();
      mTree->Sort();
      unsigned after = mTree->NumNodes();
      Log("Before sort CPTree had %u nodes, after sort %u\n", before, after);
      ASSERT(mTree->IsSorted());
      count = 0;
    }
    FPTreeFunctor::OnLoad(txn);
  }

  void OnUnload(const std::vector<Item>& txn) {
    ASSERT(mIsStreaming); // Should only be called in streaming mode.
    if (mIsStreaming) {
      std::vector<Item> t(txn);
      mTree->SortTransaction(t);
      mTree->Remove(t);
    }
  }

  unsigned interval;
  unsigned count;
};
