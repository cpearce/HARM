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

#ifndef __ITEM_H__
#define __ITEM_H__

#include <string>
#include "debug.h"
#include <stdint.h>

// Represents a single item. Internally stored as an int.
class Item {
public:
  Item();
  Item(const std::string& aName);
  Item(const char* aName);
  Item(int aItemId);
  ~Item();

  bool operator==(Item i) const {
    return mId == i.mId;
  }
  Item& operator=(const Item& i) {
    mId = i.mId;
    return *this;
  }
  bool operator<(const Item i) const;
  operator int() const {
    return mId;
  }
  operator std::string() const;

  static Item FromIndex(unsigned index) {
    return Item(index + 1);
  }

  int GetId() const {
    return mId;
  }

  uint32_t GetIndex() const {
    ASSERT(!IsNull());
    return static_cast<uint32_t>(mId) - 1;
  }

  bool IsNull() const {
    return mId == 0;
  }

  // Clear the comparison lookup table, and resets the base item Id.
  // Note this will break all existing Items, use with care!
  // You may choose to call this when you load a new data set for a
  // fresh run, and you want to minimze the impact of caching the
  // comparisons. You'd need a *huge* number of types of items for
  // it to make an impact though!
  static void ResetBaseId();

  // Sets the comparison mode to either alphabetic or insertion/encounter-order
  // mode. In insertion order mode, an item encountered before another item is
  // considered "less than" that item. Note alphabetic mode is much slower.
  enum CompareMode {
    INSERTION_ORDER_COMPARE, // Default.
    ALPHABETIC_COMPARE
  };
  static void SetCompareMode(CompareMode aMode);

  const char* GetName() const;

private:
  void Init(const std::string& aName);
  int32_t mId;
};

#endif
