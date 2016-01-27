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
  for_each(index.begin(), index.end(), [&](vector<bitset<chunk_size>>& vec) {
    vec.reserve(index_reserved_chunks);
  });
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
  // Note: We do not increment iterator, so we do the existance in the
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
void EnsureCapacity(vector<T>& vec, uint32_t size) {
  if (vec.size() < size + 1) {
    if (vec.capacity() < size + 1) {
      // Increase the capacity by 20%. This is to prevent thrashing as
      // we resize and add more transactions.
      vec.reserve(unsigned(vec.capacity() * 1.2) + 1u);
    }
    vec.resize(size + 1);
  }
}

// Appends a transaction to the end of the sliding window.
void VariableWindowDataSet::Append(const Transaction& transaction) {
  auto offset = transaction.id - first_chunk_start_tid;
  auto chunk_idx = offset / chunk_size;
  auto bit_num = offset % chunk_size;
  for_each(transaction.items.begin(), transaction.items.end(), [&](Item item) {
    // Get the transaction id list, where this item is present.
    auto item_idx = item.GetIndex();

    EnsureCapacity(index, item_idx);
    TidList& tidlist = index.at(item_idx);

    EnsureCapacity(tidlist, chunk_idx);
    std::bitset<chunk_size>& chunk = tidlist.at(chunk_idx);

    chunk.set(bit_num, 1);
  });
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

// Makes a Transaction for testing.
// Transaction have their tid=n.
// All Transactions contain Item("a").
// Transactions with tid divisible by 2 contain Item("b").
// Transactions with tid divisible by 3 contain Item("c").
static Transaction MakeTestTransaction(uint32_t n) {
  string items("a");
  if ((n % 2) == 0) {
    items += ",b";
  }
  return Transaction(n, items);
}

#ifdef _DEBUG
// Returns the number of transactions which contain Item("a") in the range
// [start_id, start_id+len]. Note [0,0] is a length 1 range.
static uint32_t TestExpectedItemA(uint32_t start_id, uint32_t len) {
  return len;
}

// Returns the number of transactions which contain Item("b") in the range
// [start_id, end_id].
static uint32_t TestExpectedItemB(uint32_t start_id, uint32_t len) {
  uint32_t count = 0;
  for (uint32_t i = start_id; i < start_id + len; i++) {
    if ((i % 2) == 0) {
      count++;
    }
  }
  return count;
}
#endif

// static
void VariableWindowDataSet::Test() {
  // Simple test of removing inside the first chunk.
  {
    VariableWindowDataSet d;
    ASSERT(d.NumTransactions() == 0);
    d.Append(Transaction(0, "a,b,e"));
    ASSERT(d.NumTransactions() == 1);
    d.Append(Transaction(1, "a,c,e"));
    ASSERT(d.NumTransactions() == 2);
    d.Append(Transaction(2, "b,c,d,e"));
    ASSERT(d.NumTransactions() == 3);
    d.Append(Transaction(3, "c,d,e,f"));
    ASSERT(d.NumTransactions() == 4);
    d.Append(Transaction(4, "e"));
    ASSERT(d.NumTransactions() == 5);

    // Ensure dataset is correctly stored.
    ASSERT(d.Count(Item("a")) == 2);
    ASSERT(d.Count(Item("b")) == 2);
    ASSERT(d.Count(Item("c")) == 3);
    ASSERT(d.Count(Item("d")) == 2);
    ASSERT(d.Count(Item("e")) == 5);
    ASSERT(d.Count(Item("f")) == 1);
    ASSERT(d.Count(Item("g")) == 0); // Invalid item.

    ASSERT(d.Count(ItemSet("a", "b")) == 1);
    ASSERT(d.Count(ItemSet("a", "e")) == 2);
    ASSERT(d.Count(ItemSet("c", "e")) == 3);
    ASSERT(d.Count(ItemSet("g", "h")) == 0); // Invalid itemset.

    //d.RemoveUpTo(0);
    d.Pop();
    ASSERT(d.NumTransactions() == 4);
    ASSERT(d.Count(Item("a")) == 1);
    ASSERT(d.Count(Item("b")) == 1);
    ASSERT(d.Count(Item("c")) == 3);
    ASSERT(d.Count(Item("d")) == 2);
    ASSERT(d.Count(Item("e")) == 4);
    ASSERT(d.Count(Item("f")) == 1);

    //d.RemoveUpTo(2);
    d.Pop();
    d.Pop();
    ASSERT(d.Count(Item("a")) == 0);
    ASSERT(d.Count(Item("b")) == 0);
    ASSERT(d.Count(Item("c")) == 1);
    ASSERT(d.Count(Item("d")) == 1);
    ASSERT(d.Count(Item("e")) == 2);
    ASSERT(d.Count(Item("f")) == 1);
    ASSERT(d.NumTransactions() == 2);

    //d.RemoveUpTo(3);
    d.Pop();
    ASSERT(d.Count(Item("a")) == 0);
    ASSERT(d.Count(Item("b")) == 0);
    ASSERT(d.Count(Item("c")) == 0);
    ASSERT(d.Count(Item("d")) == 0);
    ASSERT(d.Count(Item("e")) == 1);
    ASSERT(d.Count(Item("f")) == 0);
    ASSERT(d.NumTransactions() == 1);
  }
  {
    ASSERT(TestExpectedItemA(0, 0) == 0);
    ASSERT(TestExpectedItemB(0, 0) == 0);
    ASSERT(TestExpectedItemB(0, 1) == 1);
    ASSERT(TestExpectedItemB(0, 2) == 1);
    ASSERT(TestExpectedItemB(0, 3) == 2);

    ASSERT(TestExpectedItemA(1, 3) == 3);
    ASSERT(TestExpectedItemB(0, 4) == 2);

    VariableWindowDataSet d;
    const uint32_t n = VariableWindowDataSet::chunk_size * 4 + 1;
    for (uint32_t i = 0; i < n; i++) {
      d.Append(MakeTestTransaction(i));
      ASSERT(d.Count(Item("a")) == TestExpectedItemA(0, i + 1));
      ASSERT(d.Count(Item("b")) == TestExpectedItemB(0, i + 1));
    }

    // Remove all the transactions, verify that the correct number of
    // items remain.
    for (uint32_t i = 0; i < n; i++) {
      Transaction t = d.Front();
      d.Pop();
      ASSERT(d.Count(Item("a")) == TestExpectedItemA(i + 1, d.NumTransactions()));
      ASSERT(d.Count(Item("a")) == d.NumTransactions());
      ASSERT(d.Count(Item("b")) == TestExpectedItemB(i + 1, d.NumTransactions()));
      ASSERT(d.Count(ItemSet("a", "b")) == TestExpectedItemB(i + 1, d.NumTransactions()));
    }
  }
  #ifdef INDEX_STRESS_TEST
  {
    cout << "Stress testing VariableWindowDataSet by loading kosarak..." << endl;
    VariableWindowDataSet d;
    DataSetReader reader;
    bool ok = reader.Open("datasets/kosarak.csv");
    ASSERT(ok);
    if (!ok) {
      cerr << "Failed to open kosarak.csv" << endl;
      return;
    }
    TransactionId tid = 0;
    while (true) {
      Transaction transaction(tid);
      if (!reader.GetNext(transaction.items)) {
        cout << "Finished, loaded " << tid << " transactions." << endl;
        break;
      }
      tid++;
      d.Append(transaction);
      if ((tid % 50000) == 0) {
        cout << "Loaded up to transaction " << tid << endl;
      }
    }
  }
  #endif
}
