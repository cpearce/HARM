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
#include "ItemSet.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

ItemSet::ItemSet() {

}


ItemSet::~ItemSet() {

}


ItemSet::ItemSet(const char** aItems, int aNumItems) {
  for (int i = 0; i < aNumItems; i++) {
    operator+=(Item(aItems[i]));
  }
}


ItemSet& ItemSet::operator+=(ItemSet& aOther) {
  set<Item>::const_iterator itr = aOther.mItems.begin();
  set<Item>::const_iterator end = aOther.mItems.end();
  while (itr != end) {
    operator+=(*itr);
    itr++;
  }
  return *this;
}

void ItemSet::Add(Item i) {
  mItems.insert(i);
}

ItemSet& ItemSet::operator+=(Item aOther) {
  Add(aOther);
  return *this;
}

bool ItemSet::operator==(const ItemSet& aOther) const {
  if (Size() != aOther.Size()) {
    return false;
  }
  set<Item>::const_iterator thisItr = mItems.begin();
  set<Item>::const_iterator otherItr = aOther.mItems.begin();
  while (thisItr != mItems.end()) {
    if (*thisItr != *otherItr) {
      return false;
    }
    thisItr++;
    otherItr++;
  }
  return true;
}

bool ItemSet::operator==(const vector<Item>& v) const {
  if (Size() != v.size()) {
    return false;
  }
  auto vs = set<Item>(v.begin(), v.end());
  return vs == mItems;
}

ItemSet::operator string() const {
  vector<string> v;
  set<Item>::const_iterator itr = mItems.begin();
  set<Item>::const_iterator end = mItems.end();
  while (itr != end) {
    Item item = *itr;
    string s = item;
    v.push_back(s);
    itr++;
  }
  sort(v.begin(), v.end());
  stringstream ss;
  for (unsigned i = 0; i < v.size(); i++) {
    ss << v[i];
    if (i + 1 < v.size()) {
      ss << " ";
    }
  }
  return ss.str();
}


ItemSet::ItemSet(const Item& aItem) {
  operator+=(aItem);
}


ItemSet::ItemSet(const char* aItem1) {
  operator+=(Item(aItem1));
}

ItemSet::ItemSet(const char* aItem1, const char* aItem2) {
  operator+=(Item(aItem1));
  operator+=(Item(aItem2));
}

ItemSet::ItemSet(const char* aItem1, const char* aItem2, const char* aItem3) {
  operator+=(Item(aItem1));
  operator+=(Item(aItem2));
  operator+=(Item(aItem3));
}

ItemSet::ItemSet(const char* aItem1, const char* aItem2, const char* aItem3, const char* aItem4) {
  operator+=(Item(aItem1));
  operator+=(Item(aItem2));
  operator+=(Item(aItem3));
  operator+=(Item(aItem4));
}

ItemSet::ItemSet(const char* aItem1, const char* aItem2, const char* aItem3, const char* aItem4, const char* aItem5) {
  operator+=(Item(aItem1));
  operator+=(Item(aItem2));
  operator+=(Item(aItem3));
  operator+=(Item(aItem4));
  operator+=(Item(aItem5));
}

bool ItemSet::operator<(const ItemSet& aOther) const {
  const size_t ourSize = Size();
  const size_t otherSize = aOther.Size();
  if (ourSize < otherSize) {
    return true;
  } else if (otherSize < ourSize) {
    return false;
  } else {
    // Compare the entries...
    set<Item>::const_iterator ourItr = mItems.begin();
    set<Item>::const_iterator ourEnd = mItems.end();
    set<Item>::const_iterator otherItr = aOther.mItems.begin();
    while (ourItr != ourEnd) {
      if ((*ourItr) != (*otherItr)) {
        return (*ourItr) < (*otherItr);
      }
      ourItr++;
      otherItr++;
    }
  }
  // Must be identical.
  return false;
}

void AddToSet(const set<Item>& aSrc, set<Item>& aDst) {
  set<Item>::const_iterator itr = aSrc.begin();
  set<Item>::const_iterator end = aSrc.end();
  while (itr != end) {
    aDst.insert(*itr);
    itr++;
  }
}

ItemSet::ItemSet(const ItemSet* o) {
  AddToSet(o->mItems, mItems);
}

ItemSet::ItemSet(const ItemSet& aItemSet1, const ItemSet& aItemSet2) {
  AddToSet(aItemSet1.mItems, mItems);
  AddToSet(aItemSet2.mItems, mItems);
}

ItemSet ItemSet::operator-(Item i) const {
  ItemSet itemset = *this;
  itemset.mItems.erase(i);
  return itemset;
}

vector<Item>
ItemSet::AsVector() const {
  vector<Item> v;
  auto itr = mItems.begin();
  auto end = mItems.end();
  while (itr != end) {
    Item item = *itr;
    v.push_back(item);
    itr++;
  }
  return v;
}

void ItemSet::Clear() {
  mItems.clear();
}

bool ItemSet::Contains(Item& i) const {
  return mItems.find(i) != mItems.end();
}


void ItemSet::Test() {
  cout << __FUNCTION__ << "()\n";

  Item dog("dog");
  Item cat("cat");

  ASSERT(cat < dog);
  ASSERT(!(dog < cat));

  ItemSet a;
  a += dog;
  a += cat;

  string str = a;
  ASSERT(str == "cat dog");

  ItemSet both("dog", "cat");
  ASSERT(both == a);

  ItemSet bothClone(&both);
  ASSERT(both == bothClone);

  ItemSet dogSet = dog;
  ASSERT(dogSet < a);

  ItemSet b;
  b += Item("budgie");
  b += Item("sheep");
  b += a;
  str = b;
  ASSERT(str == "budgie cat dog sheep");
  const char* items[] = {"sheep", "cat", "budgie", "dog"};
  ASSERT(ItemSet(items, 4) == b);

  ItemSet c("budgie", "sheep");
  ItemSet ac(a, c);
  str = ac;
  ASSERT(str == "budgie cat dog sheep");

  ItemSet d("budgie", "cat", "dog", "sheep");
  ItemSet e("budgie", "lion", "dog", "sheep");
  ASSERT(d < e);
  ItemSet ed(e, d);
  str = ed;
  ASSERT(str == "budgie cat dog lion sheep");
  ItemSet de(d, e);
  str = de;
  ASSERT(str == "budgie cat dog lion sheep");
  ASSERT(de == ed);

  Item sheep("sheep");
  ItemSet k = de - sheep;
  // Make sure the "-" operator didn't change the original.
  str = de;
  ASSERT(str == "budgie cat dog lion sheep");
  // Ensure the "-" worked...
  str = k;
  ASSERT(str == "budgie cat dog lion");

}
