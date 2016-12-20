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
#include <set>
#include <iostream>
#include <memory>

#include "CoocurrenceGraph.h"
#include "Options.h"
#include "utils.h"
#include "ItemSet.h"
#include "DataSetReader.h"

class TidList;

// Uncomment to stress test indexes in their static Test() functions
// by loading Kosarak. It takes a while, so it's not on by default.
//#define INDEX_STRESS_TEST

class LoadFunctor {
public:
  virtual void OnStartLoad(std::unique_ptr<DataSetReader>& aReader) {};

  // Called when a transaction is loaded into the dataset.
  virtual void OnLoad(const std::vector<Item>& txn) = 0;

  // Called when a transaction is unloaded from the dataset.
  // Only datasets with an eject policy (WindowIndex) will call this.
  virtual void OnUnload(const std::vector<Item>& txn) = 0;

  // Called after the load is complete.
  virtual void OnEndLoad() = 0;

  // Define a virtual destructor, so that classes that override this class
  // get their destructor's called when they're deleted!
  virtual ~LoadFunctor() {}
};

class DataSet {
protected:

  DataSet(std::unique_ptr<DataSetReader> aReader, LoadFunctor* aFunctor)
    : mFunctor(aFunctor)
    , mReader(move(aReader)) {
  }

  LoadFunctor* mFunctor;
  std::unique_ptr<DataSetReader> mReader;

public:

  void SetLoadListener(LoadFunctor* aListener) {
    mFunctor = aListener;
  }

  virtual ~DataSet() {
    delete mFunctor;
  }

  // Loads a static DataSet. Suitable for mining fixed sized data sets.
  // f can be NULL.
  virtual bool Load() = 0;

  // Loads a DataSet with a sliding window of size aWindowSize,
  // suitable for use in stream mining. aFunctor is a pointer to an object
  // which is called upon every transaction load, and can be null.
  // aNumItems (optional) suggests the number of items in the dataset.
  // For optimal performance, make aNumItems the exact number of items
  // in the dataset.
  static DataSet* Load(std::string& aFileName,
                       unsigned aWindowSize,
                       LoadFunctor* aFunctor,
                       unsigned aNumItems = 100);

  virtual int Count(const ItemSet& aItemSet) const = 0;
  virtual int Count(const Item& aItem) const = 0;

  double Support(const ItemSet& aItemSet) const {
    return (double)Count(aItemSet) / (double)NumTransactions();
  }

  double Support(const Item& aItem) const {
    return (double)Count(aItem) / (double)NumTransactions();
  }

  double Confidence(const Item antecedent, const Item consequent) {
    int count_a = Count(antecedent);
    if (count_a == 0) {
      return 0.0;
    }
    double sup_a = count_a / (double)NumTransactions();
    return (double)Support(ItemSet(antecedent, consequent)) / sup_a;
  }

  virtual unsigned NumTransactions() const = 0;

  // Returns true if the dataset has finished loading.
  virtual bool IsLoaded() const = 0;
};

class InvertedDataSetIndex : public DataSet {
public:
  InvertedDataSetIndex(std::unique_ptr<DataSetReader> aReader, LoadFunctor* f = NULL);
  virtual ~InvertedDataSetIndex();

  bool Load() override;

  int Count(const ItemSet& aItemSet) const override;
  int Count(const Item& aItem) const override;

  unsigned NumTransactions() const  override {
    return mNumTransactions;
  }
  unsigned GetTID() const {
    return mTxnId;
  }
  const std::set<Item>& GetItems() const;
  unsigned GetNumItems() const {
    return (unsigned)mInvertedIndex.size();
  }

  bool IsLoaded() const override;

protected:

  // Maps Item Id to list of transactions containing that item.
  std::map<int, TidList> mInvertedIndex;
  unsigned mNumTransactions = 0;
  unsigned mTxnId = 0;
  bool mLoaded = false;

  // Stores each item in the data set.
  std::set<Item> mItems;

  void GetTidLists(const ItemSet& aItemSet,
                   std::vector<const TidList*>& aTidLists) const;

  void GetTidLists(const ItemSet& aItemSet,
                   std::vector<int>** aShortestTidList,
                   std::vector<std::vector<int>*>& aTidLists);
};

int IntersectionSize(const ItemSet& aItem1, const ItemSet& aItem2);

