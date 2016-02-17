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

InvertedDataSetIndex::InvertedDataSetIndex(const char* aFileName, LoadFunctor* aFunctor)
  : DataSet(aFileName, aFunctor),
    mNumTransactions(0),
    mTxnId(0),
    mLoaded(false) {
  Item::ResetBaseId();
  gIndexCount++;
  ASSERT(gIndexCount == 1);
}

InvertedDataSetIndex::~InvertedDataSetIndex() {
  gIndexCount--;
  ASSERT(gIndexCount == 0);
}

bool InvertedDataSetIndex::Load() {
  cout << "Loading data file: '" << mFile << "'\n";
  DurationTimer timer;

  mInvertedIndex.clear();
  mItems.clear();
  mNumTransactions = 0;
  mTxnId = 0;
  mLoaded = false;

  DataSetReader reader;
  if (!reader.Open(mFile)) {
    cerr << "Can't open " << mFile << " failing!" << endl;
    return false;
  }

  if (mFunctor) {
    mFunctor->OnStartLoad();
  }

  vector<Item> transaction;
  while (reader.GetNext(transaction)) {
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

void InvertedDataSetIndex_Test() {
  cout << __FUNCTION__ << "()\n";
  {
    InvertedDataSetIndex index("NotARealFileName");
    ASSERT(!index.Load());
  }
  {
    /*
      Test: Dataset 1, check loading, test supports.
      a,b,c,d,e,f
      g,h,i,j,k,l
      z,x
      z,x
      z,x,y
      z,x,y,i
    */
    InvertedDataSetIndex index("datasets/test/test1.data");
    ASSERT(index.Load());
    ASSERT(index.Count(Item("a")) == 1);
    ASSERT(index.Count(Item("b")) == 1);
    ASSERT(index.Count(Item("c")) == 1);
    ASSERT(index.Count(Item("d")) == 1);
    ASSERT(index.Count(Item("e")) == 1);
    ASSERT(index.Count(Item("f")) == 1);
    ASSERT(index.Count(Item("h")) == 1);
    ASSERT(index.Count(Item("i")) == 2);
    ASSERT(index.Count(Item("j")) == 1);
    ASSERT(index.Count(Item("k")) == 1);
    ASSERT(index.Count(Item("l")) == 1);
    ASSERT(index.Count(Item("z")) == 5);
    ASSERT(index.Count(Item("x")) == 4);
    ASSERT(index.Count(Item("y")) == 2);

    int sup_zx = index.Count(ItemSet("z", "x"));
    ASSERT(sup_zx == 4);

    int sup_zxy = index.Count(ItemSet("z", "x", "y"));
    ASSERT(sup_zxy == 2);

    int sup_zxyi = index.Count(ItemSet("z", "x", "y", "i"));
    ASSERT(sup_zxyi == 1);

    // Test InvertedDataSetIndex.GetItems().
    const char* test1Items[] =
    {"a", "b", "c", "d", "e", "f", "h", "i", "j", "k", "l", "z", "x", "y"};
    const set<Item>& items = index.GetItems();
    const int numItems = sizeof(test1Items) / sizeof(const char*);
    ASSERT(numItems == 14);
    ASSERT(items.size() == numItems);
    set<Item>::const_iterator itr = items.begin();
    ASSERT((itr = items.find(Item("NotAnItem"))) == items.end());
    ASSERT((itr = items.find(Item("AlsoNotAnItem"))) == items.end());
    for (int i = 0; i < numItems; i++) {
      ASSERT((itr = items.find(Item(test1Items[i]))) != items.end());
    }
  }
  {
    // Test partially dirty data, tests parsing/tokenizing.
    /*
      aa, bb, cc , dd
      aa, gg,
      aa, bb
    */
    InvertedDataSetIndex index1("datasets/test/test2.data");
    ASSERT(index1.Load());
    ASSERT(index1.Count(Item("aa")) == 3);
    ASSERT(index1.Count(ItemSet("aa", "bb")) == 2);
    ASSERT(index1.Count(Item("gg")) == 1);
    ASSERT(index1.Count(ItemSet("bollocks", "nothing")) == 0);

    // Test IntersectionSize().
    // int IntersectionSize(const ItemSet& aItem1, const ItemSet& aItem2)
    ItemSet xyz("x", "y", "z");
    ItemSet wxy("w", "x", "y");
    ASSERT(IntersectionSize(xyz, wxy) == 2);
    ASSERT(IntersectionSize(wxy, xyz) == 2);

    const char* data_abxusji[] = {"a", "b", "x", "u", "s", "j", "i"};
    ItemSet abxusji(data_abxusji, sizeof(data_abxusji) / sizeof(const char*));
    const char* data_ndlashyilw[] = {"n", "d", "l", "a", "s", "h", "y", "i", "l", "w"};
    ItemSet ndlashyilw(data_ndlashyilw, sizeof(data_ndlashyilw) / sizeof(const char*));
    ASSERT(IntersectionSize(abxusji, ndlashyilw) == 3);
  }

  {
    InvertedDataSetIndex index1("datasets/test/UCI-zoo.csv");
    // hair = 1 count == 43
    ASSERT(index1.Load());
    ASSERT(index1.Count(Item("hair=1")) == 43);
    ASSERT(index1.Count(ItemSet("airbourne=0", "backbone=1", "venomous=0")) == 61);

  }

  {
    /*
      aa, bb, cc , dd
      aa, gg,
      aa, bb
    */
    InvertedDataSetIndex index1("datasets/test/test2.data");
    ASSERT(index1.Load());
    ASSERT(index1.Count(Item("aa")) == 3);
    ASSERT(index1.Count(ItemSet("aa", "bb")) == 2);
    ASSERT(index1.Count(Item("gg")) == 1);
    ASSERT(index1.Count(ItemSet("bollocks", "nothing")) == 0);
    ASSERT(index1.NumTransactions() == 3);
  }
}
