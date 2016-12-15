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

#include "debug.h"
#include "CoocurrenceGraph.h"
#include "ItemSet.h"
#include <iostream>

using namespace std;

CoocurrenceGraph::CoocurrenceGraph() {
}

CoocurrenceGraph::~CoocurrenceGraph() {

}

template<class T>
static void EnsureCapacity(vector<T>& aVec, unsigned aCapacity) {
  while (aVec.size() <= aCapacity) {
    aVec.resize((int)(aVec.size() * 1.5 + 1));
  }
}

// Increments the number of times a and b occur together by 1.
void CoocurrenceGraph::PartialIncrement(Item a, Item b) {
  #ifdef _DEBUG
  unsigned before = CoocurrenceCount(a, b);
  #endif

  map<Item, unsigned>* counts = GetNeighbourhoodAllocateIfNecessary(a);
  unsigned& count = (*counts)[b];
  count += 1;

  #ifdef _DEBUG
  unsigned after = CoocurrenceCount(a, b);
  ASSERT(before + 1 == after);
  #endif
}

void CoocurrenceGraph::Increment(Item a, Item b) {
  ASSERT(a != b);
  PartialIncrement(a, b);
  PartialIncrement(b, a);
}

map<Item, unsigned>*
CoocurrenceGraph::GetNeighbourhoodAllocateIfNecessary(Item a) {
  EnsureCapacity(mCooccurrenceCounts, a.GetIndex() + 1);
  if (mCooccurrenceCounts[a.GetIndex()] == nullptr) {
    auto counts = new map<Item, unsigned>();
    mCooccurrenceCounts[a.GetIndex()] = counts;
    return counts;
  }
  return mCooccurrenceCounts[a.GetIndex()];
}


map<Item, unsigned>*
CoocurrenceGraph::GetNeighbourhood(Item a) const {
  if (a.GetIndex() >= mCooccurrenceCounts.size()) {
    return nullptr;
  }
  return mCooccurrenceCounts[a.GetIndex()];
}


void CoocurrenceGraph::PartialDecrement(Item a, Item b) {
  #ifdef _DEBUG
  unsigned before = CoocurrenceCount(a, b);
  #endif
  map<Item, unsigned>* counts = GetNeighbourhood(a);
  ASSERT(counts);
  if (!counts) {
    return;
  }
  auto itr = counts->find(b);
  unsigned& count = itr->second;
  if (count == 1) {
    counts->erase(itr);
    if (counts->size() == 0) {
      mCooccurrenceCounts[a.GetIndex()] = nullptr;
      delete counts;
    }
  } else {
    count--;
  }
  #ifdef _DEBUG
  unsigned after = CoocurrenceCount(a, b);
  ASSERT(before == after + 1);
  #endif
}

void CoocurrenceGraph::Decrement(Item a, Item b) {
  ASSERT(a != b);
  PartialDecrement(a, b);
  PartialDecrement(b, a);
}

// Returns number of times 'a' and 'b' occur together in the graph.
unsigned CoocurrenceGraph::CoocurrenceCount(Item a, Item b) const {
  map<Item, unsigned>* counts = GetNeighbourhood(a);
  if (!counts) {
    return 0;
  }

  auto b_itr = counts->find(b);
  if (b_itr != counts->end()) {
    return b_itr->second;
  }
  return 0;
}

void
CoocurrenceGraph::Increment(const vector<Item>& itemset) {
  auto aItr = itemset.begin();
  auto end = itemset.end();
  while (aItr != end) {
    Item a = *aItr;
    aItr++;
    // For every subsequent item...
    auto bItr = aItr;
    while (bItr != end) {
      Item b = *bItr;
      Increment(a, b);
      bItr++;
    }
  }
}

void
CoocurrenceGraph::Decrement(const vector<Item>& itemset) {
  auto aItr = itemset.begin();
  auto end = itemset.end();
  while (aItr != end) {
    Item a = *aItr;
    aItr++;
    // For every subsequent item...
    auto bItr = aItr;
    while (bItr != end) {
      Item b = *bItr;
      Decrement(a, b);
      bItr++;
    }
  }
}

void
CoocurrenceGraph::GetNeighbourhood(const Item a, vector<Item>& n)  const {
  map<Item, unsigned>* counts = GetNeighbourhood(a);
  if (!counts) {
    return;
  }
  for (auto itr = counts->begin();
       itr != counts->end();
       itr++) {
    Item neighbour = itr->first;
    string s = neighbour;
    unsigned coccurrenceCount = itr->second;
    ASSERT(coccurrenceCount > 0);
    if (coccurrenceCount > 0) {
      n.push_back(neighbour);
    }
  }
}
