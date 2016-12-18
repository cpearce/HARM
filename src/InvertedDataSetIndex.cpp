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


#include "InvertedDataSetIndex.h"
#include "WindowIndex.h"
#include "ItemSet.h"
#include "TidList.h"
#include "debug.h"
#include "utils.h"
#include "DataSetReader.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

// We can only handle 1 index at a time.
static unsigned gIndexCount = 0;

InvertedDataSetIndex::InvertedDataSetIndex(unique_ptr<DataSetReader> aReader,
                                           LoadFunctor* aFunctor)
  : DataSet(move(aReader), aFunctor)
{
  Item::ResetBaseId();
  gIndexCount++;
  ASSERT(gIndexCount == 1);
}

InvertedDataSetIndex::~InvertedDataSetIndex() {
  gIndexCount--;
  ASSERT(gIndexCount == 0);
}

bool InvertedDataSetIndex::Load() {
  cout << "Loading data set" << endl;
  DurationTimer timer;

  mInvertedIndex.clear();
  mItems.clear();
  mNumTransactions = 0;
  mTxnId = 0;
  mLoaded = false;

  if (!mReader->IsGood()) {
    cerr << "ERROR: Can't read input;failing!" << endl;
    return false;
  }

  if (mFunctor) {
    mFunctor->OnStartLoad(mReader);
  }

  vector<Item> transaction;
  while (mReader->GetNext(transaction)) {
    ++mNumTransactions;
    ++mTxnId;
    for (unsigned i = 0; i < transaction.size(); ++i) {
      Item item = transaction[i];
      int id = item.GetId();
      TidList& tidList = mInvertedIndex[id];
      tidList.Set(mTxnId, true);
    }
    if (mFunctor) {
      mFunctor->OnLoad(transaction);
    }
  }

  // Initialize the set of items.
  map<int, TidList>::iterator itr = mInvertedIndex.begin();
  while (itr != mInvertedIndex.end()) {
    Item i((*itr).first);
    mItems.insert(i);
    itr++;
  }
  ASSERT(mItems.size() == GetNumItems());

  mLoaded = true;
  if (mFunctor) {
    mFunctor->OnEndLoad();
  }

  cout << "Loading took " << timer.Seconds() << "s.\n";
  cout << "Num transactions: " << mNumTransactions << "\n";
  cout << "Num items: " << mInvertedIndex.size() << "\n";

  return true;
}

bool InvertedDataSetIndex::IsLoaded() const {
  return mLoaded;
}

void InvertedDataSetIndex::GetTidLists(const ItemSet& aItemSet,
                                       vector<const TidList*>& aTidLists) const {
  set<Item>::const_iterator itr = aItemSet.mItems.begin();
  while (itr != aItemSet.mItems.end()) {
    int itemId = *itr;
    map<int, TidList>::const_iterator t = mInvertedIndex.find(itemId);
    if (t != mInvertedIndex.end()) {
      aTidLists.push_back(&(t->second));
    }
    itr++;
  }
}

// Counts the number of transactions which contain all items in the Itemset.
// Works by getting the TidLists of all the items, then ANDing them together,
// and counting the bits set.
int InvertedDataSetIndex::Count(const ItemSet& aItemSet) const {

  vector<const TidList*> tidLists;
  GetTidLists(aItemSet, tidLists);

  if (tidLists.empty()) {
    return 0;
  }

  int count = 0;
  const TidList* first = tidLists[0];

  vector<vector<unsigned>*> firstTidlist = first->mChunks;

  // For each chunk in the first TID list...
  for (unsigned f = 0; f < firstTidlist.size(); f++) {
    // Extract the vector of chunks.
    vector<unsigned>* chunks = firstTidlist[f];
    if (!chunks) {
      continue;
    }

    // Logical AND it with the corresponding bitset from the TidList
    // for all other itemsets.
    for (unsigned i = 0; i < chunks->size(); ++i) {
      unsigned chunk = chunks->at(i);
      if (chunk) {
        for (unsigned t = 1; t < tidLists.size(); t++) {
          chunk &= tidLists[t]->GetChunk(f, i);
        }
      }
      // Count the bits set, the number of transactions in which all the
      // items in itemset occur.
      count += PopulationCount(chunk);
    }
  }
  return count;
}

int InvertedDataSetIndex::Count(const Item& aItem) const {
  return Count(ItemSet(aItem));
}

const set<Item>& InvertedDataSetIndex::GetItems() const {
  ASSERT(mItems.size() == GetNumItems());
  return mItems;
}

int IntersectionSize(const ItemSet& aItem1, const ItemSet& aItem2) {
  set<Item>::const_iterator itr = aItem1.mItems.begin();
  set<Item>::const_iterator end = aItem1.mItems.end();
  int count = 0;
  while (itr != end) {
    if (aItem2.mItems.find(*itr) != aItem2.mItems.end()) {
      count++;
    }
    itr++;
  }
  return count;
}
