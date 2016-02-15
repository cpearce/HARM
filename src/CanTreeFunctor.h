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
#include <algorithm>

class CanTreeFunctor : public FPTreeFunctor {
public:
  CanTreeFunctor(FPTree* aTree,
                 const std::vector<unsigned>& aTxnNums,
                 int aBlockSize,
                 bool aIsStreaming,
                 DataSet* aIndex,
                 Options& aOptions)
    : FPTreeFunctor(aTree, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions) {
  }

  void OnLoad(const std::vector<Item>& txn) {
    std::vector<Item> t(txn);
    AppearanceCmp cmp;
    sort(t.begin(), t.end(), cmp);
    ASSERT(VerifySortedByAppearance(t));
    mTree->Insert(t);
    FPTreeFunctor::OnLoad(txn);
  }

  void OnUnload(const std::vector<Item>& txn) {
    ASSERT(mIsStreaming); // Should only be called in streaming mode.
    AppearanceCmp cmp;
    std::vector<Item> t(txn);
    sort(t.begin(), t.end(), cmp);
    mTree->Remove(t);
  }
};
