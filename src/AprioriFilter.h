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


#ifndef __APRIORI_FILTER_H__
#define __APRIORI_FILTER_H__

#include "InvertedDataSetIndex.h"
#include "utils.h"
#include "debug.h"

// Abstract class, used by Apriori algorithm to filter out ItemSets
// during candidate generation.
class AprioriFilter {
public:
  // Returns true if |aItem| should be kept and passed into the next
  // generation.
  virtual bool Filter(ItemSet& aItem) const = 0;
};


class AlwaysAcceptFilter : public AprioriFilter {
public:
  bool Filter(ItemSet& aItem) const override
  {
    return true;
  }
};


class MinSupportFilter : public AprioriFilter {
public:
  MinSupportFilter(double aMinSup, InvertedDataSetIndex& aIndex)
    : mMinSup(aMinSup)
    , mIndex(aIndex)
  {
  }

  bool Filter(ItemSet& aItem) const override
  {
    return mIndex.Support(aItem) >= mMinSup;
  }

private:
  double mMinSup;
  InvertedDataSetIndex& mIndex;
};


class MinCountFilter : public AprioriFilter {
public:
  MinCountFilter(int aMinCount, InvertedDataSetIndex& aIndex)
    : mMinCount(aMinCount)
    , mIndex(aIndex)
  {
  }

  bool Filter(ItemSet& aItem) const override
  {
    return mIndex.Count(aItem) >= mMinCount;
  }

private:
  int mMinCount;
  InvertedDataSetIndex& mIndex;
};

class MinAbsSupFilter : public AprioriFilter {
public:
  MinAbsSupFilter(InvertedDataSetIndex& aIndex)
    : mIndex(aIndex)
  {
  }

  bool Filter(ItemSet& aItem) const override;

private:
  InvertedDataSetIndex& mIndex;
};

#endif
