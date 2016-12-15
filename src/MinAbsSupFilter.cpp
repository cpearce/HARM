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

#include "utils.h"
#include "Item.h"
#include "ItemSet.h"
#include "InvertedDataSetIndex.h"
#include "AprioriFilter.h"
#include "debug.h"
#include <math.h>
#include <set>
#include <algorithm>

using namespace std;

static Item GetItemWithLowestSupport(ItemSet& aItemSet, InvertedDataSetIndex& aIndex) {
  set<Item>::const_iterator itr = aItemSet.mItems.begin();
  set<Item>::const_iterator end = aItemSet.mItems.end();
  ASSERT(itr != end);
  Item item;
  Item minimum = *itr;
  int lowest = aIndex.Count(item);
  itr++;
  while (itr != end) {
    item = *itr;
    int count = aIndex.Count(item);
    if (count < lowest) {
      lowest = count;
      minimum = item;
    }
    itr++;
  }
  return minimum;
}

double factorial(double n) {
  double r = 1;
  for (int i = 2; i < n; i++) {
    r *= i;
  }
  return r;
}

double lfactorial(int n) {
  double r = 0;
  for (int i = 2; i <= n; i++) {
    r += log((double)i);
  }
  return r;
}

static double pval(int AB, int A, int B, int N) {
  double logsum = lfactorial(B)
                  + lfactorial(N - B)
                  + lfactorial(A)
                  + lfactorial(N - A)
                  - lfactorial(AB)
                  - lfactorial(B - AB)
                  - lfactorial(A - AB)
                  - lfactorial(N - A - B + AB)
                  - lfactorial(N);
  return exp(logsum);
}

int MinAbsSup(int A, int B, int N, double E) {
  int limit = min(A, B);
  double sum = 0;
  int AB = max(0, A + B - N);
  for (; AB < limit; AB++) {
    sum += pval(AB, A, B, N);
    if (sum > E) {
      return AB;
    }
  }
  return limit;
}

bool MinAbsSupFilter::Filter(ItemSet& aItemSet) {
  // Get constituent item with lowest support
  Item a = GetItemWithLowestSupport(aItemSet, mIndex);
  ItemSet b = aItemSet - a;
  double minAbsSupValue = MinAbsSup(mIndex.Count(a),
                                    mIndex.Count(b),
                                    mIndex.NumTransactions(),
                                    0.999);
  return mIndex.Count(aItemSet) > minAbsSupValue;
}

