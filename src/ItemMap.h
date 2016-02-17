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

#ifndef __ITEM_MAP_H__
#define __ITEM_MAP_H__

#include "Item.h"
#include <vector>

// Maps items to arbitrary thing. Exploits the fact that item id's start at
// 1 and go to n for n items, so can use a vector for fast lookup.
template<class T>
class ItemMap {
public:

  ItemMap(const ItemMap& other)
    : v(other.v)
    , valid(other.valid) {
  }

  ItemMap() { }

  bool IsEmpty() const {
    return v.empty() && valid.empty();
  }

  bool Contains(Item item) const {
    if (item.IsNull()) {
      return false;
    }
    if ((unsigned)item.GetId() > valid.size()) {
      return false;
    }
    return valid.at(item.GetIndex());
  }

  void Set(Item key, T value) {
    ASSERT(!key.IsNull());
    unsigned index = key.GetIndex();
    if (index >= v.size()) {
      v.resize(index + 1);
      valid.resize(index + 1);
    }
    ASSERT(valid.size() == v.size());
    v[index] = value;
    valid[index] = true;
    //    ASSERT(Get(key) == value);
  }

  // Behaviour undefined if key has not been added to ItemMap.
  T Get(Item key) const {
    ASSERT(Contains(key));
    unsigned index = key.GetIndex();
    ASSERT(index < v.size());
    return v.at(index);
  }

  T Get(Item key, T fallback) const {
    if (!Contains(key)) {
      return fallback;
    }
    unsigned index = key.GetIndex();
    ASSERT(index < v.size());
    return v.at(index);
  }

  const T& GetRef(Item& key) const {
    return v.at(key.GetIndex());
  }

  void Erase(Item key) {
    ASSERT(Contains(key));
    unsigned index = key.GetIndex();
    //    v[index] = (T)0;
    valid[index] = false;
  }

  void Clear() {
    v.clear();
    valid.clear();
  }

  class Iterator {
    friend class ItemMap;
  public:

    bool HasNext() {
      while (index < v.size() && !valid[index]) {
        index++;
      }
      return (unsigned)index < v.size();
    }

    void Next() {
      if (index >= v.size()) {
        return;
      }
      do {
        index++;
      } while (index < v.size() && !valid[index]);
    }

    Item GetKey() {
      return Item((int)index + 1);
    }

    T GetValue() {
      return v[index];
    }

  private:
    Iterator(const std::vector<T>& _v, const std::vector<bool>& _valid)
      : v(_v), valid(_valid), index(0) {}

    const std::vector<T>& v;
    const std::vector<bool>& valid;
    unsigned index;
  };

  Iterator GetIterator() const {
    return Iterator(v, valid);
  }

  void Add(const ItemMap<T>& other) {
    auto itr = other.GetIterator();
    while (itr.HasNext()) {
      Set(itr.GetKey(), Get(itr.GetKey(), 0) + itr.GetValue());
      itr.Next();
    }
  }

  void Remove(const ItemMap<T>& other) {
    auto itr = other.GetIterator();
    while (itr.HasNext()) {
      auto val = Get(itr.GetKey(), 0);
      ASSERT(val >= itr.GetValue());
      Set(itr.GetKey(), val - itr.GetValue());
      itr.Next();
    }
  }

private:
  std::vector<T> v;
  std::vector<bool> valid;
};

void ItemMap_Test();

#endif
