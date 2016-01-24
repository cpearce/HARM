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

#pragma once

#include "FPTree.h"
#include "FPNode.h"

#include <vector>

#include "dlib/svm.h" //dlib library for kernel regression

using dlib::matrix;
using dlib::radial_basis_kernel;
using dlib::polynomial_kernel;
using dlib::krls;


class Interpolation {
public:
  Interpolation(FPNode* root)
    : fptree(root) {
  }

  // Called after a txn has been inserted.
  void Adjust(const std::vector<Item>& txn, bool useKernel) {
    for (unsigned i = 0; i < txn.size(); ++i) {
      Item item = txn[i];
      if (!lookup.Contains(item)) {
        unsigned index = ranking.size();
        ranking.push_back(item);
        ASSERT(ranking[index] == item);
        lookup.Set(item, index);
        ASSERT(ranking[index] == item);

        if (!useKernel) {
          storedPrevCount.Set(item, 0);
          storedInitCount.Set(item, 0);
        } else {

        }
        storedInterpolation.Set(item, 0);
      }
    }
  }

  void UpdatePrevCount() { // Used by Linear Interpolation to update counts
    for (unsigned i = 0; i < ranking.size(); i++) {
      Item item = ranking[i];
      unsigned count = fptree->freq->Get(item, 0);
      unsigned prevCount = storedPrevCount.Get(item);
      storedInitCount.Set(item, prevCount);
      storedPrevCount.Set(item, count);
    }
  }

  void StoreDataPointCount(unsigned tid) { // Used by Kernel Regression to store counts of items at specified data points
    ItemMap<unsigned> ptCount;
    for (unsigned i = 0; i < ranking.size(); i++) {
      Item item = ranking[i];
      unsigned count = fptree->freq->Get(item, 0);
      ptCount.Set(item, count);
    }
    kernelPointIDs.push_back(tid);
    storedKernelCounts.Set(tid, ptCount);
  }

  double hoeffdingBound(int blockSize, double delta) {
    double eta = 0.0;
    double bounds = log(1 / delta) / (2.0 * (double)blockSize);
    eta = sqrt(bounds);
    return eta;
  }

  void UpdateInterpolation(unsigned tid, int blockSize, bool useKernel) { //main method for Interpolation

    if (!useKernel) { // Default linear interpolation
      for (unsigned i = 0; i < ranking.size(); i++) {
        Item item = ranking[i];
        unsigned initCount = storedInitCount.Get(item);
        unsigned prevCount = storedPrevCount.Get(item);
        unsigned count = fptree->freq->Get(item, 0);
        double interpolation = 0.0;
        double firsthalve = (double)(prevCount - initCount);
        double sechalve = (double)(count - prevCount);
        double delta = 2 * (sechalve) / (double)blockSize - 2 * firsthalve / (double) blockSize;

        if (hoeffdingBound(blockSize, 0.001) < delta) {
          interpolation = (double)(count + count - initCount) / blockSize;
        } else {
          interpolation = (double)(count + 2 * sechalve - firsthalve) / blockSize;
        }
        storedInterpolation.Set(item, interpolation);
      }

    } else {	// Interpolation using Kernel Regression
      for (unsigned i = 0; i < ranking.size(); i++) {
        Item item = ranking[i];
        double interpolation = 0.0;

        typedef matrix<double, 1, 1> sample_type;
        typedef radial_basis_kernel<sample_type> kernel_type;
        typedef polynomial_kernel<sample_type> polykr_type;
        //					krls<kernel_type> kernelFunction(kernel_type(0.01),0.001,blockSize);
        krls<polykr_type> kernelFunction(polykr_type(0.01, 0, 2), blockSize);
        sample_type m;

        for (unsigned i = 0; i < kernelPointIDs.size(); i++) {
          m(0) = kernelPointIDs[i];
          kernelFunction.train(m, storedKernelCounts.Get(kernelPointIDs[i]).Get(item, 0));
        }
        m(0) = blockSize * 2;
        interpolation = kernelFunction(m);

        //				Log("%i,%lf \n",item.GetId(),interpolation);
        //				Log("%i,%lf \n",item.GetId(),(double)count/(double)blockSize);
        storedInterpolation.Set(item, interpolation);
      }
    }
  }

  void ResetInterpolation(bool useKernel) {
    for (unsigned i = 0; i < ranking.size(); i++) {
      Item item = ranking[i];
      ASSERT(lookup.Get(item) == i);
      storedInterpolation.Set(item, 0);
      if (!useKernel) {
        UpdatePrevCount();
      }

    }
  }


  ItemMap <double> GetInterpolationList() const {
    return storedInterpolation;
  }

private:

  unsigned windowSize(unsigned tid, int blockSize) {
    if (blockSize == 0) {
      return tid;
    }
    return tid > blockSize ? (blockSize) : (tid);
  }

  std::vector<Item> ranking;

  // Maps an item to its index in the ranking vector.
  ItemMap<unsigned> lookup;
  ItemMap<double> storedInterpolation;

  ItemMap<unsigned> storedPrevCount;
  ItemMap<unsigned> storedInitCount;

  ItemMap<ItemMap<unsigned>> storedKernelCounts;
  std::vector<unsigned> kernelPointIDs;

  FPNode* fptree;
};


class ExtrapTreeFunctor : public FPTreeFunctor {
public:
  ExtrapTreeFunctor(FPNode* aTree,
                    unsigned aThreshold,
                    int aBlockSize,
                    bool aIsStreaming,
                    const std::vector<unsigned>& aTxnNums,
                    DataSet* aIndex,
                    bool useKernelRegression,
                    int aDataPoints,
                    Options& aOptions)
    : FPTreeFunctor(aTree, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions),
      interval(aThreshold),
      numOfNodes(1),
      ranks(aTree),
      n(1),
      count(0),
      windowSize(aBlockSize),
      hasMlist(false),
      useKernel(useKernelRegression),
      numOfKernelPoints(aDataPoints),
      i_count(0) {
  }

  struct InterpolationAppearanceCmp : public ItemComparator {
    InterpolationAppearanceCmp(const ItemMap<double>& f) : freq(f) {}
    bool operator()(const Item a, const Item b) {
      if (!freq.Contains(a) && freq.Contains(b)) {
        return false;
      }
      if (freq.Contains(a) && !freq.Contains(b)) {
        return true;
      }
      if ((!freq.Contains(a) && !freq.Contains(b)) || freq.Get(a) == freq.Get(b)) {
        return a.GetId() < b.GetId();
      }
      return freq.Get(a) > freq.Get(b);
    }
    const ItemMap<double>& freq;
  };


  void OnLoad(const std::vector<Item>& txn) {
    std::vector<Item> t(txn);

    if (hasMlist) {
      sort(t.begin(), t.end(), InterpolationAppearanceCmp(mList));
    } else {
      sort(t.begin(), t.end(), AppearanceCmp());
      ASSERT(VerifySortedByAppearance(t));
    }

    mTree->Insert(t);

    ranks.Adjust(t, useKernel);

    count++;

    // kernel regression
    if (useKernel && ((count % (windowSize / numOfKernelPoints)) == 0)) {

      ranks.StoreDataPointCount(count);
    }

    if (count == windowSize) {
      Log("Sorting ExtrapTree at transaction %u\n", mTxnNum);
      int before = mTree->NumNodes();
      //    if (((double)(before - numOfNodes)/(double)numOfNodes) > 0.01) { //delay method
      ranks.UpdateInterpolation(n, windowSize, useKernel);
      mList = ranks.GetInterpolationList();
      InterpolationAppearanceCmp cmp(mList);
      mTree->Sort(&cmp);
      int after = mTree->NumNodes();
      Log("Before sort Extraptree had %u nodes, after sort %u\n", before, after);
      numOfNodes = after;
      ranks.ResetInterpolation(useKernel);
      hasMlist = true;
      //     } else { //delay method
      //       Log("Sorting delayed had %u nodes \n", before) ; //delay method
      //		numOfNodes = before;
      //     } // delay method

      count = 0;
      i_count++;
    }

    // linear interpolation
    if (!useKernel && interval / 2 == count) {
      ranks.UpdatePrevCount();
    }

    FPTreeFunctor::OnLoad(txn);
    n++;
  }

  void OnUnload(const std::vector<Item>& txn) {
    ASSERT(mIsStreaming); // Should only be called in streaming mode.
    std::vector<Item> t(txn);

    if (hasMlist) {
      sort(t.begin(), t.end(), InterpolationAppearanceCmp(mList));
    } else {
      sort(t.begin(), t.end(), AppearanceCmp());
      ASSERT(VerifySortedByAppearance(t));
    }
    mTree->Remove(t);
  }

  ItemMap<double> mList;
  unsigned interval;
  unsigned numOfNodes;
  Interpolation ranks;
  unsigned n;
  unsigned count;
  int windowSize;
  bool hasMlist;
  bool useKernel;
  unsigned numOfKernelPoints;

  int i_count;
};
