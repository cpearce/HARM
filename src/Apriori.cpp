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
#include <future>

using namespace std;

bool ContainsAllSubSets(const set<ItemSet>& aContainer, const ItemSet& aItemSet) {
  ItemSet k(aItemSet);
  for (Item item : aItemSet.mItems) {
    k.mItems.erase(item);
    if (aContainer.find(k) == aContainer.end()) {
      // Can't find the subset in the container, fail.
      return false;
    }
    k.mItems.insert(item);
  }
  return true;
}

static void
safeAdvance(set<ItemSet>::const_iterator& aIter,
            const set<ItemSet>::const_iterator end,
            uint32_t aStep)
{
  for (uint32_t i = 0; i < aStep && aIter != end; i++) {
    aIter++;
  }
}

set<ItemSet>
GenerateSubCandidates(const set<ItemSet>& aCandidates,
                      uint32_t aStartOffset,
                      uint32_t aItemSetSize,
                      uint32_t aStep,
                      shared_ptr<AprioriFilter> aFilter)
{
  set<ItemSet> result;
  set<ItemSet>::const_iterator iter = aCandidates.cbegin();
  safeAdvance(iter, aCandidates.end(), aStartOffset);
  for (; iter != aCandidates.end(); safeAdvance(iter, aCandidates.end(), aStep)) {
    set<ItemSet>::const_iterator y = aCandidates.begin();
    for (const ItemSet& y : aCandidates) {
      const ItemSet& x = *iter;
      if (IntersectionSize(x, y) == aItemSetSize - 2) {
        ItemSet itemset(x, y);
        if (aFilter->Filter(itemset) && ContainsAllSubSets(aCandidates, itemset)) {
          result.insert(move(itemset));
        }
      }
    } // for y
  } // for x
  return result;
}

set<ItemSet>
GenerateCandidates(const set<ItemSet>& aCandidates,
                   int aItemSetSize,
                   shared_ptr<AprioriFilter> aFilter,
                   int numThreads)
{
  const uint32_t cores = numThreads;
  vector<future<set<ItemSet>>> futures;
  set<ItemSet>::iterator end = aCandidates.end();
  for (uint32_t i = 0; i < cores; i++) {
    auto future = std::async(std::launch::async,
                             GenerateSubCandidates,
                             aCandidates, i, aItemSetSize, cores, aFilter);
    futures.push_back(move(future));
  }

  set<ItemSet> results;
  for (future<set<ItemSet>>& future : futures) {
    const set<ItemSet>& result = future.get();
    results.insert(result.begin(), result.end());
  }
 
  return results;
}

set<ItemSet>
GenerateInitialCandidates(const InvertedDataSetIndex& aIndex,
                          shared_ptr<AprioriFilter> aFilter) {
  set<ItemSet> candidates;
  for (const Item item : aIndex.GetItems()) {
    ItemSet itemset = item;
    if (aFilter->Filter(itemset)) {
      candidates.insert(move(itemset));
    }
  }
  return candidates;
}

void Apriori(Options& options) {
  InvertedDataSetIndex index(make_unique<DataSetReader>(make_unique<ifstream>(options.inputFileName)));
  index.Load();

  vector<ItemSet> result;

  int k = 1;
  shared_ptr<AprioriFilter> filter = nullptr;
  if (options.mode == kApriori) {
    filter = make_shared<MinSupportFilter>(options.minSup, index);
  } else if (options.mode == kMinAbssup) {
    filter = make_shared<MinAbsSupFilter>(index);
  }

  Log("Generating initial candidates...\n");
  set<ItemSet> candidates = GenerateInitialCandidates(index, filter);
  result.insert(result.end(), candidates.begin(), candidates.end());

  Log("Generated %u candidates\n", candidates.size());

  time_t aprioriStartTime = time(0);

  while (candidates.size() != 0) {
    time_t startTime = time(0);
    k++;
    candidates = GenerateCandidates(candidates, k, filter, options.numThreads);
    result.insert(result.end(), candidates.begin(), candidates.end());

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
  Log("Generating rules...\n");
  DurationTimer timer;
  GenerateRules(result, 0.9, 1.0, numRules, &index, options.outputFilePrefix, options.countRulesOnly);
  Log("Generated %d rules in %.3lfs...\n", numRules, timer.Seconds());
}
