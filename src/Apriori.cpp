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

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "utils.h"
#include "Item.h"
#include "ItemSet.h"
#include "InvertedDataSetIndex.h"
#include "AprioriFilter.h"
#include "TidList.h"
#include "debug.h"
#include <thread>
#include "Options.h"
#include <sstream>

using namespace std;

bool ContainsAllSubSets(const set<ItemSet>& aContainer, const ItemSet& aItemSet) {
  ItemSet k(aItemSet);

  set<Item>::const_iterator itr = aItemSet.mItems.begin();
  set<Item>::const_iterator end = aItemSet.mItems.end();
  for (itr; itr != end; itr++) {
    Item item = *itr;
    k.mItems.erase(item);
    if (aContainer.find(k) == aContainer.end()) {
      // Can't find the subset in the container, fail.
      return false;
    }
    k.mItems.insert(item);
  }
  return true;
}

void
GenCanWorker(const set<ItemSet>& aCandidates,
             set<ItemSet>& aResult,
             int aItemSetSize,
             AprioriFilter* aFilter,
             bool aOdd)
{
  int mask = aOdd ? 1 : 0;

  set<ItemSet>::const_iterator x = aCandidates.begin();
  set<ItemSet>::const_iterator end = aCandidates.end();
  int counter = 0;
  for (x; x != end; x++, counter++) {

    // Skip this one if it's being done by the other worker thread.
    if ((counter & 1) == mask) {
      continue;
    }

    set<ItemSet>::const_iterator y = aCandidates.begin();
    for (y; y != end; y++) {
      if (IntersectionSize((*x), (*y)) == aItemSetSize - 2) {
        ItemSet itemset((*x), (*y));
        if (aFilter->Filter(itemset) && ContainsAllSubSets(aCandidates, itemset)) {
          aResult.insert(itemset);
        }
      }
    } // for y
  } // for x
}

void GenerateCandidates(const set<ItemSet>& aCandidates,
                        set<ItemSet>& aResult,
                        int aItemSetSize,
                        AprioriFilter* aFilter,
                        int numThreads) {

  if (numThreads > 1) {

    // Just do dual-threaded approach for now...
    // TODO: Implement for more threads...
    set<ItemSet> evenResults;
    std::thread even(&GenCanWorker, 
                     aCandidates,
                     std::ref(evenResults),
                     aItemSetSize,
                     aFilter,
                     false);

    set<ItemSet> oddResults;
    std::thread odd(&GenCanWorker,
                    aCandidates,
                    std::ref(oddResults),
                    aItemSetSize,
                    aFilter,
                    true);
    even.join();
    odd.join();

    cout << "evenResults.size()=" << evenResults.size() << endl;
    cout << "oddResults.size()=" << oddResults.size() << endl;

    aResult.insert(evenResults.begin(), evenResults.end());
    aResult.insert(oddResults.begin(), oddResults.end());

  } else {

    // Single threaded approach.
    set<ItemSet>::const_iterator x = aCandidates.begin();
    set<ItemSet>::const_iterator end = aCandidates.end();
    for (x; x != end; x++) {
      set<ItemSet>::const_iterator y = aCandidates.begin();
      for (y; y != end; y++) {
        if (IntersectionSize((*x), (*y)) == aItemSetSize - 2) {
          ItemSet itemset((*x), (*y));
          if (aFilter->Filter(itemset) && ContainsAllSubSets(aCandidates, itemset)) {
            aResult.insert(itemset);
          }
        }
      } // for y
    } // for x

  }
}

void GenerateInitialCandidates(const InvertedDataSetIndex& aIndex,
                               AprioriFilter& aFilter,
                               set<ItemSet>& aCandidates) {
  const set<Item>& items = aIndex.GetItems();
  set<Item>::const_iterator itr = items.begin();
  while (itr != items.end()) {
    Item item = *itr;
    itr++;
    ItemSet itemset = item;
    if (aFilter.Filter(itemset)) {
      aCandidates.insert(itemset);
    }
  }
}

void Apriori(Options& options) {

  // Fix up so it uses sets or whatever.
  InvertedDataSetIndex index(make_unique<DataSetReader>(make_unique<ifstream>(options.inputFileName)));
  index.Load();

  vector<ItemSet> result;

  int k = 1;
  AprioriFilter* filter = nullptr;
  if (options.mode == kApriori) {
    filter = new MinSupportFilter(options.minSup, index);
  } else if (options.mode == kMinAbssup) {
    filter = new MinAbsSupFilter(index);
  }

  set<ItemSet> candidates;
  set<ItemSet> temp;

  Log("Generating initial candidates...\n");
  GenerateInitialCandidates(index, *filter, candidates);
  result.insert(result.end(), candidates.begin(), candidates.end());

  Log("Generated %u candidates\n", candidates.size());

  time_t aprioriStartTime = time(0);

  while (candidates.size() != 0) {
    time_t startTime = time(0);
    k++;
    GenerateCandidates(candidates, temp, k, filter, options.numThreads);
    result.insert(result.end(), temp.begin(), temp.end());
    candidates = temp;
    temp.clear();

    time_t timeTaken = (time(0) - startTime);
    Log("=============\n");
    Log("Finished generating itemsets of size %u\n", k);
    Log("Generated %u itemsets\n", candidates.size());
    Log("Time for generation: %lld\n", timeTaken);
  }

  Log("Apriori finished, generated %u itemsets in %llds\n",
      result.size(), (time(0) - aprioriStartTime));

  DumpItemSets(result, &index, options);

  vector<Rule> rules;
  long numRules = 0;
  GenerateRules(result, 0.9, 1.0, numRules, &index, options.outputFilePrefix, options.countRulesOnly);

  delete filter;
}
