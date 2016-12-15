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


#ifndef __COOCCURRENCE_GRAPH_H__
#define __COOCCURRENCE_GRAPH_H__

#include "Item.h"
#include "ItemMap.h"

#include <vector>
#include <map>

class CoocurrenceGraph {
public:
  CoocurrenceGraph();
  ~CoocurrenceGraph();

  void Increment(const std::vector<Item>& itemset);
  void Decrement(const std::vector<Item>& itemset);

  // Returns number of times 'a' and 'b' occur together in the graph.
  unsigned CoocurrenceCount(Item a, Item b) const;

  // Adds all items that co-occur with a into n
  void GetNeighbourhood(const Item a, std::vector<Item>& n) const;

private:

  // Increments the number of times a and b occur together by 1.
  void Increment(Item a, Item b);
  void Decrement(Item a, Item b);

  std::map<Item, unsigned>* GetNeighbourhood(Item a) const;

  std::map<Item, unsigned>* GetNeighbourhoodAllocateIfNecessary(Item a);

  // Increments b in a's cooccurrence list.
  void PartialIncrement(Item a, Item b);

  // Decrements b in a's cooccurrence list.
  void PartialDecrement(Item a, Item b);

  // Maps an item A to the another map which maps the other items that
  // co-occur with A to their frequency of co-occurence.
  std::vector<std::map<Item, unsigned>*> mCooccurrenceCounts;

};

#endif
