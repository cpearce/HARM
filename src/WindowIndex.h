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

#include <map>
#include <vector>
#include <queue>
#include <set>

#include "Item.h"
#include "InvertedDataSetIndex.h"
#include "ItemMap.h"

class Item;
class ItemSet;

class WindowIndex : public DataSet {
public:
  // windowLength: Length of the sliding window in transactions.
  // numItems: Suggested amount of space to reserve for the number of items in
  //   the inverted index. For optimal performance, make this the exact number
  //   of unique items in the data set.
  WindowIndex(const char* aFile,
              LoadFunctor* aFunctor,
              unsigned windowLength,
              unsigned numItems = 100);
  ~WindowIndex();

  bool Load();

  int Count(const ItemSet& aItemSet);
  int Count(const Item& aItem);
  unsigned NumTransactions() const;

  bool IsLoaded() const override;

  static void Test();

private:

  // Records whether aItem's chunk with index aChunkIndex is used or
  // not. We use this information to optimize the Count() function.
  void MarkChunkUsage(Item aItem, unsigned aChunkIndex, bool aUsed);

  unsigned GetNumActiveChunks(Item aItem);

  void Set(unsigned aTid, Item aItem, bool aValue);
  bool Get(unsigned aTid, Item aItem) const;

  void VerifyWindow(unsigned aWindowFrontTxnNum) const;

  std::set<Item> mItems;

  // Maps item index to the set of the indexes of chunks which contain
  // non-zero tids. This is used to reduce the number of chunks which
  // need to be iterated over in Count().
  std::vector<std::set<unsigned>> mActiveChunks;

  // Transactions in the window.
  std::queue<std::vector<Item>> mWindow;

  // Inverted index. First dimension is itemId, second dimension is transaction id.
  // i.e. (mIndex[item][tid/sizeof(unsigned)] & (tid % sizeof(unsigned)))
  std::vector<unsigned*> mIndex;

  // Maximum length of the window, i.e. the max number transactions in the window.
  // Size of mIndex's second dimension.
  unsigned mMaxLength;

  // Id of the "largest" item in the dataset, so we can iterate over all items
  // in the index easily.
  int mMaxItemId;

  // Whether the dataset has finished streaming.
  bool mLoaded;
};
