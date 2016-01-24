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
#include <set>
#include <algorithm>
#include <assert.h>

class DiscTreeFunctor : public FPTreeFunctor,
  public ItemFilter {
public:
  DiscTreeFunctor(FPNode* aTree,
                  unsigned aSortThreshold,
                  int aBlockSize,
                  bool aIsStreaming,
                  const std::vector<unsigned>& aTxnNums,
                  DataSet* aIndex,
                  Options& aOptions)
    : FPTreeFunctor(aTree, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions),
      sortInterval(aSortThreshold),
      haveSetupDiscriminativeness(false),
      count(0),
      min_discriminativeness(aOptions.min_discriminativeness)
  {
  }

  void OnLoad(const std::vector<Item>& txn) {

    std::vector<Item> t(txn);
    EnsureSorted(t);

    coocurrences.Increment(t);

    // Keep track of the items the occur in the data set.
    std::for_each(t.begin(), t.end(), [&](Item x) {
      uint32_t count = mItemFreq.Get(x, 0);
      if (count == 0) {
        // We're adding this for the first time.
        mItems.insert(x);
      }
      mItemFreq.Set(x, count + 1);
    });

    mTree->Insert(t);
    count++;

    if (count == sortInterval) {
      Log("Sorting Discriminatory Tree at transaction %u\n", mTxnNum);

      // First re-calculate the discriminativeness for each item still
      // in the dataset.

      // foreach item X
      std::for_each(mItems.begin(), mItems.end(), [&](Item x) {
        // foreach item Y connected to item X
        std::vector<Item> x_neighbourhood;
        coocurrences.GetNeighbourhood(x, x_neighbourhood);
        double d = 0;
        double g = 0;
        double supx = mIndex->Support(x);
        int count = 0;
        std::for_each(x_neighbourhood.begin(), x_neighbourhood.end(), [&](Item y) {
          count++;
          //	double y_neighbourhood_gain = 0;
          double supxy = mIndex->Support(ItemSet(x, y));
          double supy = mIndex->Support(y);
          g += -1 * (supxy) * log((supxy) / supx) - ((supx - supxy)) * log((supx - supxy + 0.000001) / supx);
          g += -1 * (supxy) * log((supxy) / supy) - ((supy - supxy)) * log((supy - supxy + 0.000001) / supy);
        });
        d = g / 2;
        discriminativeness.Set(x, d);
      });

      haveSetupDiscriminativeness = true;

      // Sort tree.
      ItemMapCmp<double> comparator(discriminativeness);
      mTree->Sort(&comparator);

      count = 0;
    }

    FPTreeFunctor::OnLoad(txn);
  }

  void OnUnload(const std::vector<Item>& txn) {
    ASSERT(mIsStreaming); // Should only be called in streaming mode.
    std::vector<Item> t(txn);
    EnsureSorted(t);
    coocurrences.Decrement(t);

    // Keep track of the items the occur in the data set.
    std::for_each(t.begin(), t.end(), [&](Item x) {
      uint32_t count = mItemFreq.Get(x, 0);
      if (count == 1) {
        // We're removing this for the last time.
        mItems.erase(x);
      }
      assert(count >= 1);
      mItemFreq.Set(x, count - 1);
    });

    mTree->Remove(t);
  }

  void EnsureSorted(std::vector<Item>& t) {
    if (haveSetupDiscriminativeness) {
      // Sort according to the descriminativeness scores.
      sort(t.begin(), t.end(), ItemMapCmp<double>(discriminativeness));
    } else {
      sort(t.begin(), t.end(), AppearanceCmp());
      ASSERT(VerifySortedByAppearance(t));
    }
  }


  // ItemFilter implementation.
  bool ShouldKeep(Item item) override {
    return !haveSetupDiscriminativeness ||
           isnan(min_discriminativeness) ||
           discriminativeness.Get(item, 0) > min_discriminativeness;
  }

protected:

  // Records frequency of individual items.
  ItemMap<unsigned> mItemFreq;
  std::set<Item> mItems;

  ItemFilter* GetItemFilter() override {
    return this;
  }

  const unsigned sortInterval;

  // True if discriminativeness is valid.
  bool haveSetupDiscriminativeness;

  // Maps item to is discriminativeness.
  ItemMap<double> discriminativeness;

  CoocurrenceGraph coocurrences;

  unsigned count;
  const double min_discriminativeness;
};

