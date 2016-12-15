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

#include "VariableWindowDataSet.h"
#include "DataSetReader.h"
#include "utils.h"

#include <algorithm>

using namespace std;

VariableWindowDataSet::VariableWindowDataSet()
  : DataSet(nullptr, nullptr),
    first_chunk_start_tid(0),
    first_chunk_start_offset(0) {
  index.reserve(index_reserved_items);
  for (vector<bitset<chunk_size>>& vec : index) {
    vec.reserve(index_reserved_chunks);
  }
}

VariableWindowDataSet::~VariableWindowDataSet() {
}

bool VariableWindowDataSet::Load() {
  // Should not be called!
  ASSERT(false);
  return false;
}

int VariableWindowDataSet::Count(const ItemSet& aItemSet) const {
  auto& items = aItemSet.mItems;

  // Find item with the shortest tidlist.
  auto itr = items.begin();
  Item smallest_item = *itr;
  // Note: We do not increment iterator, so we do the existence in the
  // loop check below.
  for (; itr != items.end(); itr++) {
    Item item = *itr;
    if (item.GetIndex() >= index.size()) {
      // Item does not exist in itemset, so it will have 0 count.
      return 0;
    }
    if (index[item.GetIndex()].size() < index[smallest_item.GetIndex()].size()) {
      smallest_item = item;
    }
  }

  // AND the shortest tidlist with all other item's tidlists, and count that.
  size_t count = 0;
  const TidList& shortest = index[smallest_item.GetIndex()];
  for (uint32_t i = 0; i < shortest.size(); i++) {
    bitset<chunk_size> b(shortest[i]);
    if (b.none()) {
      // No bits set in this chunk, no point iterating over other chunks, as
      // the result will be 0 when we AND with them.
      continue;
    }
    for (auto itr = items.begin(); itr != items.end(); itr++) {
      if (*itr == smallest_item) {
        continue;
      }
      const TidList& other = index[itr->GetIndex()];
      if (i < other.size()) {
        b &= other[i];
      }
    }
    count += b.count();
  }
  return (int)count;
}

int VariableWindowDataSet::Count(const Item& aItem) const {
  auto item_idx = aItem.GetIndex();
  if (item_idx >= index.size()) {
    return 0;
  }
  const TidList& tidlist = index.at(item_idx);
  size_t count = 0;
  for (auto i = 0; i < tidlist.size(); i++) {
    const std::bitset<chunk_size>& b = tidlist[i];
    count += b.count();
  }
  return (int)count;
}

unsigned VariableWindowDataSet::NumTransactions() const {
  return unsigned(transactions.size());
}

bool VariableWindowDataSet::IsLoaded() const {
  // Should not be called!
  ASSERT(false);
  return false;
}

template<class T>
void EnsureCapacity(vector<T>& vec, uint32_t size, uint32_t grow) {
  if (vec.size() < size + 1) {
    if (vec.capacity() < size + 1) {
      // Increase the capacity by 200%. This is to prevent thrashing as
      // we resize and add more transactions.
      vec.reserve(vec.capacity() + grow);
    }
    vec.resize(size + 1);
  }
}

// Appends a transaction to the end of the sliding window.
void VariableWindowDataSet::Append(const Transaction& transaction) {
  auto offset = transaction.id - first_chunk_start_tid;
  auto chunk_idx = offset / chunk_size;
  auto bit_num = offset % chunk_size;
  for (Item item : transaction.items) {
    // Get the transaction id list, where this item is present.
    EnsureCapacity(index, item.GetIndex(), index_reserved_chunks);
    TidList& tidlist = index.at(item.GetIndex());
    EnsureCapacity(tidlist, chunk_idx, index_reserved_chunks);
    std::bitset<chunk_size>& chunk = tidlist.at(chunk_idx);

    chunk.set(bit_num, 1);
  }
  transactions.push_back(transaction);
}

const Transaction& VariableWindowDataSet::Front() {
  return transactions.front();
}

const Transaction& VariableWindowDataSet::Back() {
  return transactions.back();
}

void VariableWindowDataSet::Pop() {
  auto& transaction = Front();
  if (first_chunk_start_offset + 1 < chunk_size) {
    // Update the inverted index to clear the bits set corresponding to
    // the items in this transaction.
    auto offset = transaction.id - first_chunk_start_tid;
    ASSERT(offset == first_chunk_start_offset);
    ASSERT((offset / chunk_size) == 0); // Should be first chunk.
    auto bit_num = offset % chunk_size;

    for (uint32_t i = 0; i < transaction.items.size(); i++) {
      Item item = transaction.items[i];
      // Assert that the bit was actually set first!
      ASSERT(index[item.GetIndex()][0][bit_num] == true);
      index[item.GetIndex()][0].set(bit_num, false);
    }
    first_chunk_start_offset++;
  } else {
    // We're removing the last transaction in the first chunk/bitset. We
    // must drop every tidlist's first chunk.
    for (uint32_t i = 0; i < index.size(); i++) {
      TidList& tidlist = index[i];
      if (tidlist.size() > 0) {
        // Some tid lists can be empty if the corresponding item only appeared
        // in blocks that have already been purged.
        tidlist.erase(tidlist.begin(), tidlist.begin() + 1);
      }
    }
    first_chunk_start_offset = 0;
    first_chunk_start_tid += chunk_size;
  }
  transactions.pop_front();
}
