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

#include "WindowIndex.h"
#include "DataSetReader.h"

#include <iostream>
#include <fstream>

#include <time.h>
#include <stdlib.h>

#include "Item.h"
#include "ItemSet.h"
#include "debug.h"
#include "utils.h"

using namespace std;

// Set to 1 to verify the window's contents against the index every
// transaction insertion. Will be slow on large datasets.
//#define VERIFY_WINDOW 1

WindowIndex::WindowIndex(const char* aFile,
                         LoadFunctor* aFunctor,
                         unsigned windowLength,
                         unsigned aNumItems)
  : DataSet(aFile, aFunctor),
    mMaxLength(windowLength),
    mMaxItemId(0),
    mLoaded(false) {
  Item::ResetBaseId();
  mIndex.reserve(aNumItems);
}

WindowIndex::~WindowIndex() {
  for (unsigned i = 0; i < mIndex.size(); ++i) {
    delete mIndex[i];
  }
  mIndex.clear();
}

static inline unsigned GetChunkIndex(unsigned aTid, unsigned aWindowLength) {
  return (aTid % aWindowLength) / (8 * sizeof(unsigned));
}

static inline unsigned GetChunkBitNum(unsigned aTid, unsigned aWindowLength) {
  return (aTid % aWindowLength) % (8 * sizeof(unsigned));
}

void WindowIndex::Set(unsigned aTid, Item aItem, bool aValue) {
  unsigned index = aItem.GetIndex();
  if (index >= mIndex.size()) {
    mIndex.resize(index + 1);
  }
  if (!mIndex[index]) {
    unsigned len = mMaxLength / (8 * sizeof(unsigned)) + 1;
    mIndex[index] = new unsigned[len];
    if (!mIndex[index]) {
      cerr << "Malloc failure constructing WindowIndex" << endl;
      exit(-1);
    }
    memset(mIndex[index], 0, sizeof(unsigned) * len);
  }
  unsigned* tidlist = mIndex[index];
  unsigned chunkIndex = GetChunkIndex(aTid, mMaxLength);
  unsigned bitNum = GetChunkBitNum(aTid, mMaxLength);
  ASSERT(bitNum < 32);
  unsigned chunk = tidlist[chunkIndex];
  unsigned chunkBefore = chunk;
  if (aValue) {
    chunk |= (1u << bitNum);
  } else {
    chunk &= (~0 - (1u << bitNum));
  }
  tidlist[chunkIndex] = chunk;
  if ((chunkBefore == 0 || chunk == 0) &&
      chunkBefore != chunk) {
    MarkChunkUsage(aItem, chunkIndex, chunk != 0);
  }
  ASSERT(mActiveChunks[aItem.GetIndex()].count(chunkIndex) > 0 == chunk > 0);
  ASSERT(Get(aTid, aItem) == aValue);
}

void WindowIndex::MarkChunkUsage(Item aItem, unsigned aChunkIndex, bool aUsed) {
  unsigned itemIndex = aItem.GetIndex();
  if (itemIndex >= mActiveChunks.size()) {
    mActiveChunks.resize(itemIndex + 1);
  }
  set<unsigned>& activeChunks = mActiveChunks[itemIndex];
  if (aUsed) {
    activeChunks.insert(aChunkIndex);
    ASSERT(activeChunks.count(aChunkIndex) == 1);
    ASSERT(mActiveChunks[itemIndex].count(aChunkIndex) == 1);
  } else {
    activeChunks.erase(aChunkIndex);
    ASSERT(activeChunks.count(aChunkIndex) == 0);
    ASSERT(mActiveChunks[itemIndex].count(aChunkIndex) == 0);
  }
}

bool WindowIndex::Get(unsigned aTid, Item aItem) const {
  if (aItem.GetIndex() >= mIndex.size() ||
      mIndex[aItem.GetIndex()] == 0) {
    return false;
  }
  unsigned* tidlist = mIndex[aItem.GetIndex()];
  unsigned index = GetChunkIndex(aTid, mMaxLength);
  unsigned bitNum = GetChunkBitNum(aTid, mMaxLength);
  unsigned chunk = tidlist[index];
  return (chunk & (1u << bitNum)) != 0;
}

void WindowIndex::VerifyWindow(unsigned aWindowFrontTxnNum) const {
#ifdef VERIFY_WINDOW
  queue<vector<Item>> window(mWindow);
  unsigned tid = aWindowFrontTxnNum;
  while (window.size()) {
    vector<Item> txn = window.front();
    window.pop();
    map<Item, bool> isSet;
    for (unsigned i = 0; i < txn.size(); ++i) {
      isSet[txn[i]] = true;
    }
    for (unsigned id = 1; id <= mMaxItemId; ++id) {
      Item item(id);
      bool value = isSet[item];
      ASSERT(Get(tid, item) == value);
    }
    ++tid;
  }
  cout << "Verified WindowIndex with window [" << aWindowFrontTxnNum <<
       "," << tid << "]" << endl;
#endif
}

bool WindowIndex::Load() {
  cout << "WindowIndex loading data file: '" << mFile << endl;

  DataSetReader reader;
  if (!reader.Open(mFile)) {
    cerr << "Can't open " << mFile << " failing!" << endl;
    exit(-1);
  }

  if (mFunctor) {
    mFunctor->OnStartLoad();
  }

  vector<Item> transaction;
  unsigned transactionNum = 0;
  while (reader.GetNext(transaction)) {
    // Transaction number, starting from 0.
    if (mWindow.size() == mMaxLength) {
      if (mFunctor) {
        mFunctor->OnUnload(mWindow.front());
      }
      unsigned frontTxnNum = transactionNum - mMaxLength;
      ASSERT((frontTxnNum % mMaxLength) == transactionNum % mMaxLength);
      // Window is full, remove first transaction in the window from the index.
      ASSERT(GetChunkBitNum(frontTxnNum, mMaxLength) == GetChunkBitNum(transactionNum, mMaxLength));
      ASSERT(GetChunkIndex(frontTxnNum, mMaxLength) == GetChunkIndex(transactionNum, mMaxLength));
      vector<Item>& txn = mWindow.front();
      for (unsigned i = 0; i < txn.size(); ++i) {
        Set(frontTxnNum, txn[i], false);
      }
      mWindow.pop();
    }
    mWindow.push(transaction);

    // Set each item's bit
    for (unsigned i = 0; i < transaction.size(); ++i) {
      Item item = transaction[i];
      Set(transactionNum, item, true);
      if (item.GetId() > mMaxItemId) {
        mMaxItemId = item.GetId();
      }
    }
    if (mFunctor) {
      mFunctor->OnLoad(transaction);
    }
    transactionNum++;
    #ifdef VERIFY_WINDOW
    if (transactionNum % mMaxLength == 0) {
      VerifyWindow(transactionNum - mWindow.size());
    }
    #endif
  }
  mLoaded = true;
  if (mFunctor) {
    mFunctor->OnEndLoad();
  }
  return true;
}

unsigned WindowIndex::GetNumActiveChunks(Item aItem) const {
  unsigned index = aItem.GetIndex();
  if (index >= mActiveChunks.size()) {
    return 0;
  }
  return mActiveChunks[index].size();
}

bool WindowIndex::IsLoaded() const {
  return mLoaded;
}

int WindowIndex::Count(const ItemSet& aItemSet) const {
  // Find the item in the set with the smallest number of non-zero chunks
  // in its bitset row.
  Item sparsest;
  unsigned sparsestNumChunks = UINT_MAX;
  {
    set<Item>::const_iterator itr = aItemSet.mItems.begin();
    set<Item>::const_iterator end = aItemSet.mItems.end();
    while (itr != end) {
      Item item = (*itr);
      unsigned numChunks = GetNumActiveChunks(item);
      if (numChunks < sparsestNumChunks) {
        sparsest = item;
        sparsestNumChunks = numChunks;
      }
      itr++;
    }
  }

  if (sparsestNumChunks == 0) {
    // One item doesn't appear anywhere! The union of all items can't
    // be more frequent!
    return 0;
  }

  // Logical AND together the chunks from the sparsest item with the other
  // item's corresponding chunks.
  int count = 0;
  unsigned* chunks = mIndex[sparsest.GetIndex()];
  if (!chunks) {
    return 0;
  }
  const set<unsigned>& sparsestChunks = mActiveChunks[sparsest.GetIndex()];
  set<unsigned>::const_iterator chunkItr = sparsestChunks.begin();
  set<unsigned>::const_iterator chunkEnd = sparsestChunks.end();
  while (chunkItr != chunkEnd) {
    unsigned chunkIndex = (*chunkItr);
    unsigned chunk = chunks[chunkIndex];
    ASSERT(chunk); // Should only store for active chunks!
    // Logical and together with all other items corresponding chunk.
    set<Item>::const_iterator itr = aItemSet.mItems.begin();
    set<Item>::const_iterator end = aItemSet.mItems.end();
    // Logical AND together each item in the set's chunks.
    while (itr != end && chunk) {
      Item item = (*itr);
      itr++;
      if (item == sparsest || !mIndex[item.GetIndex()]) {
        continue;
      }
      chunk &= mIndex[item.GetIndex()][chunkIndex];
    }
    count += PopulationCount(chunk);
    chunkItr++;
  }
  return count;
}

int WindowIndex::Count(const Item& aItem) const {
  return Count(ItemSet(aItem));
}

unsigned WindowIndex::NumTransactions() const {
  return mWindow.size();
}
