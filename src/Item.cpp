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
#include "utils.h"
#include "Item.h"
#include "ItemMap.h"

#include <map>
#include <iostream>
#include <bitset>
#include <math.h>

using namespace std;

map<string, int> gItemNameToId;
ItemMap<string> gIdToItemName;
static int gItemIdCount = 1;

static Item::CompareMode sCmpMode = Item::INSERTION_ORDER_COMPARE;

/* static */
void Item::SetCompareMode(CompareMode aMode) {
  sCmpMode = aMode;
}

Item::Item()
  : mId(0) {
}

Item::Item(int aItemId)
  : mId(aItemId) {
}

Item::Item(const char* aName) {
  string name(aName);
  Init(name);
}

Item::Item(const string& aName) {
  Init(aName);
}

void Item::Init(const string& aName) {
  string name = TrimWhiteSpace(aName);
  int itemId = gItemNameToId[name];
  if (itemId == 0) {
    itemId = gItemIdCount++;
    gItemNameToId[name] = itemId;
    gIdToItemName.Set(Item(itemId), name);
  }
  mId = itemId;
}

Item::~Item() {

}

Item::operator string() const {
  if (mId == 0 || !gIdToItemName.Contains(*this)) {
    return "null";
  }
  return gIdToItemName.Get(*this);
}

void Item::ResetBaseId() {
  gItemNameToId.clear();
  gItemIdCount = 1;
  gIdToItemName.Clear();
}

// Operator less than...
bool Item::operator<(Item const aItem) const {
  int other = aItem;
  ASSERT(mId > 0);
  ASSERT(other > 0);
  ASSERT(sCmpMode == INSERTION_ORDER_COMPARE ||
         sCmpMode == ALPHABETIC_COMPARE);
  switch (sCmpMode) {
    case INSERTION_ORDER_COMPARE:
      return mId < other;
    case ALPHABETIC_COMPARE:
      string us = operator string();
      string them = aItem;
      return us < them;
  }
  return false;
}
