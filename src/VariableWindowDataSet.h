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

#include "InvertedDataSetIndex.h"
#include "DataStreamMining.h"

#include <bitset>
#include <deque>

// A DataSet that has a variable length. This is different from WindowIndex
// in that it's efficient to remove chunks of transactions from the front,
// and loading is not controlled by this DataSet.
class VariableWindowDataSet : public DataSet {
public:

  VariableWindowDataSet();
  ~VariableWindowDataSet();

  // DataSet overrides.
  bool Load() override; // Not implemented.
  int Count(const ItemSet& aItemSet) const override;
  int Count(const Item& aItem) const override;
  unsigned NumTransactions() const override;
  bool IsLoaded() const override; // Not implemented.

  // Appends a transaction to the end of the sliding window.
  void Append(const Transaction& transaction);

  const Transaction& Front();
  const Transaction& Back();

  void Pop();

  static void Test();

  // Size (in bits) for each bitset chunk in the inverted index.
  static const uint32_t chunk_size = 128;

private:

  // Number of items that we reserve space for in the inverted index.
  static const uint32_t index_reserved_items = 1024;
  // Number of transactions that we reserve space for, for each item's tid list
  // in the index.
  static const uint32_t index_reserved_chunks = 100000 / chunk_size;

  typedef std::vector<std::bitset<chunk_size>> TidList;

  // Inverted index.
  // First dimension is indexed by item id.
  // Second dimension is indexed by TransactionId/chunk_size.
  // Bitset is indexed by TransactionId%chunk_size.
  // The constructor reserves space in this vector (to prevent resizing) to
  // store tid lists of length index_reserved_chunks*chunk_size
  // transactions, for index_reserved_items items.
  std::vector<TidList> index;

  // We cache transactions here, so that we can remove them
  std::deque<Transaction> transactions;

  // The transaction id of the bit at index 0 in every tidlist.
  TransactionId first_chunk_start_tid;

  // The index of the first valid bit in the first chunk.
  uint32_t first_chunk_start_offset;
};