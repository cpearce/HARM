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

#ifndef __ITEMSET_H__
#define __ITEMSET_H__

#include "Item.h"
#include <string>
#include <set>
#include <vector>

class ItemSet {
public:
  ItemSet();
  // Convenience constructors, mostly for testing.
  ItemSet(const char* aItem);
  ItemSet(const char* aItem1, const char* aItem2);
  ItemSet(const char* aItem1, const char* aItem2, const char* aItem3);
  ItemSet(const char* aItem1, const char* aItem2, const char* aItem3, const char* aItem4);
  ItemSet(const char* aItem1, const char* aItem2, const char* aItem3, const char* aItem4, const char* aItem5);
  ItemSet(const char** aItems, int aNumItems);
  ItemSet(const Item& aItem);
  // Merges two itemsets.
  ItemSet(const ItemSet& aItemSet1, const ItemSet& aItemSet2);
  // Clones an itemset.
  ItemSet(const ItemSet* o);
  ~ItemSet();

  bool IsNull() const {
    return Size() == 0;
  }
  int Size() const {
    return (int)mItems.size();
  }
  void Add(Item i);
  void Clear();

  ItemSet& operator+=(ItemSet& i);
  ItemSet& operator+=(Item i);
  ItemSet operator-(Item i) const;
  operator std::string() const;
  bool operator==(const ItemSet& i) const;
  bool operator==(const std::vector<Item>& v) const;
  bool operator<(const ItemSet& i) const;

  static void Test();

  std::vector<Item> AsVector() const;

  bool Contains(Item&) const;


  std::set<Item> mItems;

};

#endif