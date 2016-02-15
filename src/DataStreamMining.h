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

#include "Item.h"
#include "utils.h"
#include <vector>

class Options;
class FPTree;
class DataSet;

void MineDataStream(const Options& options);

typedef uint32_t TransactionId;

// A transaction represents a bunch of items that appear together, along
// with a unique id. The id is monotonically increasing, starting from 0.
class Transaction {
public:
  Transaction(TransactionId _id)
    : id(_id) {
  }

  // Helper for testing. Example usage:
  //  Transaction(0, "a,b,c")
  Transaction(TransactionId _id, std::string _items)
    : id(_id),
      items(ToItemVector(_items)) {
  }
  const TransactionId id;
  std::vector<Item> items;
};

// Class that abstracts the details for mining rules using FPGrowth.
// StreamMiner implementations can use this to mine rules, without having
// to worry about specifying output filenams etc.
//
// Do not inherit from this class. To add new mining modes, implement a new
// StreamMiner interface.
class MiningContext {
public:
  MiningContext(const Options& options);
  void Mine(FPTree* aTree, DataSet* dataset);
private:
  const Options& options;
  uint32_t mining_run;
};


// The various modes of stream mining must implement this interface.
// MineDataStream() reads the data set, and calls StreamMiner::Add() on the
// streaming modes' StreamMiner implementation when a new transaction is
// encoutered. The StreamMiner must mine the rules when it determines that
// it's appropriate to do so.
class StreamMiner {
public:
  virtual void Init(MiningContext* miner) = 0;
  virtual void Add(Transaction& transaction) = 0;
};
