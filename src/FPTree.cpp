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

#include "FPTree.h"

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <float.h>
#include <math.h>
#include "utils.h"
#include "FPNode.h"
#include "ItemSet.h"
#include "InvertedDataSetIndex.h"
#include "AprioriFilter.h"
#include "TidList.h"
#include "debug.h"
#include "CoocurrenceGraph.h"
#include "Options.h"
#include <sstream>
#include "PatternStream.h"
#include "WindowIndex.h"
#include "DataSetReader.h"
#include "DataStreamMining.h"
#include <queue>

#include "CanTreeFunctor.h"
#include "CPTreeFunctor.h"
#include "SpoTreeFunctor.h"
#include "ExtrapTreeFunctor.h"
#include "DDTreeFunctor.h"
#include "DiscTreeFunctor.h"

using namespace std;

// Comparison in non-increasing order of frequency, tie-breaking on appearance
// in dataset order. i.e. items with higher frequency are considered "greater",
// tie break so that items which were encountered earlier are considered "greater".
struct FreqCmp : public ItemComparator {
  FreqCmp(const ItemMap<unsigned>& f) : freq(f) {}
  bool operator()(const Item a, const Item b) override {
    ASSERT(freq.Contains(a));
    ASSERT(freq.Contains(a));
    return freq.Get(a) > freq.Get(b);
  }
  const ItemMap<unsigned>& freq;
};

void AddPatternsInPath(const FPNode* tree,
                       PatternOutputStream& output,
                       vector<Item>& pattern,
                       unsigned maxNodeDepth = UINT32_MAX,
                       ItemFilter* filter = nullptr) {
  ASSERT(tree != 0);
  ASSERT(!tree->IsRoot());

  if (tree->depth >= maxNodeDepth) {
    return;
  }
  // tODO: check in options that we're not using tree prunde depth and
  //       disctree filtering at thte same time.
  const bool shouldInclude = tree->depth < maxNodeDepth &&
                             (!filter || filter->ShouldKeep(tree->item));

  if (shouldInclude) {
    pattern.push_back(tree->item);
    // Record the pattern including this iten.
    output.Write(pattern);
    pattern.pop_back();
  }

  FPNode* child = tree->FirstChild();
  // Terminate recursion if no more children.
  if (!child) {
    return;
  }

  // Recurse without adding this node's item to prefix.
  AddPatternsInPath(child, output, pattern, maxNodeDepth, filter);

  if (shouldInclude) {
    // Recurse with adding this node's item to prefix.
    pattern.push_back(tree->item);
    AddPatternsInPath(child, output, pattern, maxNodeDepth, filter);
    pattern.pop_back();
  }
}

bool VerifySortedByAppearance(vector<Item>& txn) {
  for (unsigned i = 1; i < txn.size(); i++) {
    int p = txn[i - i].GetId();
    int t = txn[i].GetId();
    if (p > t) {
      return false;
    }
  }
  return true;
}

void ConstructConditionalTree(const FPNode* node,
                              FPTree* tree,
                              double minCount,
                              unsigned nodePruneDepth = std::numeric_limits<unsigned>::max()) {
  // Create a "projection" of the database, where we only have itemsets
  // from the conditional pattern base in it. We need to count frequencies
  // of all items in the conditional pattern base for this.
  ItemMap<unsigned> freq;

  const FPNode* n = node;
  while (n) {
    if (n->depth >= nodePruneDepth) {
      n = n->next;
      continue;
    }
    #ifdef _DEBUG
    string s = n->item;
    #endif
    unsigned count = n->count;
    //freq[n->item] += count;
    // Determine the items which are in this path.
    FPNode* p = n->parent;
    #ifdef _DEBUG
    string ps = p ? (string)p->item : "N/A";
    #endif
    while (p && !p->IsRoot()) {
      freq.Set(p->item, freq.Get(p->item, 0) + count);
      p = p->parent;
    }
    n = n->next;
  }

  // For each node in the headerTable (that is, for each instance of the
  // item in the tree).
  FreqCmp cmp(freq);
  while (node) {
    if (node->depth >= nodePruneDepth) {
      node = node->next;
      continue;
    }
    #ifdef _DEBUG
    string s = node->item;
    #endif
    // Determine the items which are in this path.
    FPNode* p = node->parent;
    unsigned count = node->count;
    vector<Item> path;
    while (p && !p->IsRoot()) {
      #ifdef _DEBUG
      string ps = p->item;
      #endif
      if (freq.Get(p->item, 0) >= minCount) {
        path.insert(path.begin(), p->item);
      }
      p = p->parent;
    }

    // Ensure items are sorted in non-increasing order of frequency, as
    // it's recorded in the freq table (the projection of the database).
    sort(path.begin(), path.end(), cmp);

    // Add them to the new tree.
    tree->Insert(path, count);

    node = node->next;
  }
}

void FPGrowth(FPTree* tree,
              PatternOutputStream& output,
              vector<Item>& pattern,
              DataSet* index,
              const double minCount,
              unsigned nodePruneDepth = std::numeric_limits<unsigned>::max(),
              ItemFilter* = nullptr);

void MineFPTree(FPTree* fptree,
                double minSup,
                const std::string& itemSetsOuputFilename,
                const std::string& rulesOuputFilename,
                DataSet* index,
                uint32_t treePruneDepth,
                bool countItemSetsOnly,
                bool countRulesOnly,
                ItemFilter* filter) {
  if (!fptree) {
    return;
  }

  double minCount = minSup * index->NumTransactions();
  Log("minCount=%lf\n", minCount);

  DurationTimer timer;

  bool writeItemSets = !countItemSetsOnly;

  PatternOutputStream output;
  if (writeItemSets) {
    shared_ptr<ostream> stream = std::make_shared<std::ofstream>(itemSetsOuputFilename);
    if (!stream->good()) {
      cerr << "FAIL: Can't open " << itemSetsOuputFilename << " for PatternStreamWriter output!" << endl;
      exit(-1);
    }
    output = move(PatternOutputStream(stream, index));
  }

  vector<Item> pattern;
  FPGrowth(fptree, output, pattern, index, minCount, treePruneDepth, filter);
  output.Close();

  Log("FPGrowth generated %lld patterns in %.3lfs%s\n",
      output.GetNumPatterns(), timer.Seconds(),
      (!writeItemSets ? " (not saved to disk)" : ""));

  if (!writeItemSets) {
    Log("Skipping rule generation because itemsets weren't saved to disk to generate from\n");
  } else {
    long numRules = 0;
    PatternInputStream input(make_shared<ifstream>(itemSetsOuputFilename));
    ASSERT(input.IsOpen());
    GenerateRules(input, 0.9, 1.0, numRules, index, rulesOuputFilename, countRulesOnly);
  }
  Log("-----------------------------------------------\n");
}

void FPGrowth(FPTree* tree,
              PatternOutputStream& output,
              vector<Item>& pattern,
              DataSet* index,
              const double minCount,
              unsigned nodePruneDepth,
              ItemFilter* filter) {
  ASSERT(tree != 0);
  if (tree->HasSinglePath()) {
    FPNode* t = tree->GetRoot()->FirstChild();
    if (t) {
      AddPatternsInPath(t, output, pattern, nodePruneDepth, filter);
    }
  } else {
    // For each item in the nodelist, construct a new tree, minus the item.
    // Then recurse on the new tree and its new nodelist.
    ItemMap<FPNode*>::Iterator itr = tree->HeaderTable().GetIterator();
    while (itr.HasNext()) {
      Item item = itr.GetKey();
      #ifdef _DEBUG
      string s = item; // So I can tell what item we're dealing with...
      #endif
      FPNode* node = itr.GetValue();
      itr.Next();

      if (filter && !filter->ShouldKeep(item)) {
        continue;
      }

      // For cantree mode, we can have items in the header table which don't
      // reach the minsup, so we must avoid creating conditional trees for
      // them here.
      if (index->Count(item) < minCount) {
        continue;
      }

      // Construct a new conditional tree.
      FPTree* subtree = new FPTree();

      // Note: we don't pass the ItemFilter to ConstructConditionalTree, as we
      // assume that anything above |item| in the tree also will be signalled to
      // be kept by the filter.
      ConstructConditionalTree(node, subtree, minCount, nodePruneDepth);
      pattern.push_back(item);
      output.Write(pattern);
      if (!subtree->IsEmpty()) {
        // Recurse.
        FPGrowth(subtree, output, pattern, index, minCount, nodePruneDepth, filter);
      }
      pattern.pop_back();
      delete subtree;
    }

  }
}

/*
  In DDTree mode fptree points to cptree and  ddtree points to extraptree
*/
LoadFunctor* CreateLoadFunctor(DataSet* index, FPTree* fptree, Options& options) {
  switch (options.mode) {
    case kFPTree:
      return new FPTreeFunctor(fptree,
                               options.logTreeTxn,
                               0,
                               false,
                               index,
                               options);
    case kCanTree:
      return new CanTreeFunctor(fptree,
                                options.logTreeTxn,
                                0,
                                false,
                                index,
                                options);
    case kCpTree:
      Log("CP Tree sort interval: %u\n", options.cpSortInterval);
      return new CpTreeFunctor(fptree,
                               options.cpSortInterval,
                               0,
                               false,
                               options.logTreeTxn,
                               index,
                               options);
    case kCpTreeStream:
      Log("CP Tree sort interval: %u\n", options.cpSortInterval);
      return new CpTreeFunctor(fptree,
                               options.cpSortInterval,
                               options.blockSize,
                               true,
                               options.logTreeTxn,
                               index,
                               options);
    case kSpoTree:
      return new SpoTreeFunctor(fptree,
                                options.spoSortThreshold,
                                0,
                                false,
                                options.logTreeTxn,
                                index,
                                options);
    case kSpoTreeStream:
      return new SpoTreeFunctor(fptree,
                                options.spoSortThreshold,
                                options.blockSize,
                                true,
                                options.logTreeTxn,
                                index,
                                options);
    case kStream:
      return new CanTreeFunctor(fptree,
                                options.logTreeTxn,
                                options.blockSize,
                                true,
                                index,
                                options);
    case kExtrapTreeStream:
      Log("Extrap Tree blocksize: %d\n", options.blockSize);
      Log("Extrap Tree minsup: %lf\n", options.minSup);
      Log("Extrap Tree using kernel regression: %d\n", options.useKernelRegression);
      if (options.useKernelRegression) {
        Log("Extrap Tree number of data points: %d\n", options.dataPoints);
      }
      return new ExtrapTreeFunctor(fptree,
                                   options.ExtrapSortThreshold,
                                   options.blockSize,
                                   true,
                                   options.logTreeTxn,
                                   index,
                                   options.useKernelRegression,
                                   options.dataPoints,
                                   options);
    case kDDTreeStream:
      Log("DD Tree blocksize: %d\n", options.blockSize);
      Log("DD Tree minsup: %lf\n", options.minSup);
      Log("DD Tree using kernel regression: %d\n", options.useKernelRegression);
      if (options.useKernelRegression) {
        Log("Extrap Tree number of data points: %d\n", options.dataPoints);
      }
      return new DDTreeFunctor(fptree,
                               options.useKernelRegression,
                               options.dataPoints,
                               index,
                               options.cpSortInterval,
                               options.blockSize,
                               true,
                               options.logTreeTxn,
                               options);
    case kDiscTree:
      return new DiscTreeFunctor(fptree,
                                 options.discSortThreshold,
                                 options.blockSize,
                                 true,
                                 options.logTreeTxn,
                                 index,
                                 options);

    default:
      return NULL;
  }
}

FPTree* CreateFPTree(DataSet* aDataSet, Options& options) {
  FPTree* fptree = new FPTree();
  aDataSet->SetLoadListener(CreateLoadFunctor(aDataSet, fptree, options));
  return fptree;
}

void FPTreeMiner(Options& options) {
  DurationTimer timer;
  const bool isStreaming = options.mode == kCpTreeStream ||
                           options.mode == kSpoTreeStream ||
                           options.mode == kExtrapTreeStream ||
                           options.mode == kDDTreeStream ||
                           options.mode == kStream ||
                           options.mode == kDiscTree ||
                           options.mode == kSSDD ||
                           options.mode == kDBDD;

  Log("\nLoading dataset into tree...\n");
  AutoPtr<DataSet> index = 0;
  auto reader = make_unique<DataSetReader>(make_unique<ifstream>(options.inputFileName));
  if (isStreaming) {
    index = new WindowIndex(move(reader), NULL, options.blockSize);
  } else {
    index = new InvertedDataSetIndex(move(reader));
  }
  FPTree* fptree = CreateFPTree(index, options);
  if (!fptree) {
    return;
  }

  // Load the dataset. If we're in a streaming mode, this will call the
  // load functor, the load functor will build the tree and mine when
  // appropriate.
  index->Load();

  unsigned numNodes = fptree->NumNodes();
  Log("Loaded dataset into tree of %u nodes in %.3lfs\n",
      numNodes, timer.Seconds());

  double minCount = options.minSup * index->NumTransactions();
  Log("minCount=%lf\n", minCount);

  string itemSetsOuputFilename = GetOutputItemsetsFileName(options.outputFilePrefix);
  string rulesOutputFilename = GetOutputRuleFileName(options.outputFilePrefix);

  if (!isStreaming) {
    // In the streaming case we mine during load. In the non streaming case
    // we need to mine at the end of loading the data set.
    MineFPTree(fptree,
               options.minSup,
               itemSetsOuputFilename,
               rulesOutputFilename,
               index,
               options.treePruneDepth,
               options.countItemSetsOnly,
               options.countRulesOnly);
  }
  delete fptree;
}


TreeMetricsLogger::TreeMetricsLogger(FPTree* _tree,
                                     const std::vector<unsigned>& _txnNums)
  : tree(_tree),
    txnNums(_txnNums),
    count(0),
    index(0) {}

void TreeMetricsLogger::OnTxn() {
  count++;
  while (index < txnNums.size() && txnNums[index] < count) {
    index++;
  }
  if (index < txnNums.size() && count == txnNums[index]) {
    Log("Tree at transaction %u has %u nodes\n",
        count, tree->NumNodes());
    index++;
  }
}

FPTreeFunctor::FPTreeFunctor(FPTree* aTree,
                             const std::vector<unsigned>& aTxnNums,
                             int aBlockSize,
                             bool aIsStreaming,
                             DataSet* aIndex,
                             Options& aOptions)
  : mTree(aTree),
    mLogger(aTree, aTxnNums),
    mIsStreaming(aIsStreaming),
    mOptions(aOptions),
    mTxnNum(0),
    mBlockSize(aBlockSize),
    mMiningRun(0),
    mIndex(aIndex) {
}

void FPTreeFunctor::OnStartLoad(unique_ptr<DataSetReader>& aReader) {
  if (mOptions.mode != kFPTree) {
    return;
  }

  // Make a pass of the data set to determine item frequencies.
  vector<Item> transaction;
  mInitialFrequencyTable.Clear();
  aReader->Rewind();
  while (aReader->GetNext(transaction)) {
    for (const Item item : transaction) {
      mInitialFrequencyTable.Increment(item, 1);
    }
  }
  aReader->Rewind();
}

void FPTreeFunctor::OnLoad(const std::vector<Item>& txn) {
  mLogger.OnTxn();

  if (mOptions.mode == kFPTree) {
    // Sort in non-increasing order by (pre-determined) item frequency.
    auto transaction = txn;

    struct Comparator {
      Comparator(const ItemMap<unsigned>& f) : freq(f) {}
      bool operator()(const Item a, const Item b) {
        assert(freq.Contains(a));
        assert(freq.Contains(a));
        return freq.Get(a) > freq.Get(b);
      }
      const ItemMap<unsigned>& freq;
    };

    sort(transaction.begin(), transaction.end(), Comparator(mInitialFrequencyTable));
    mTree->Insert(transaction);
  }

  mTxnNum++;
  if (mIsStreaming && (mTxnNum % mBlockSize) == 0) {
    Log("Mining rules at txnNum=%d\n", mTxnNum);
    mMiningRun++;
    string itemSetsOuputFilename =
      GetOutputItemsetsFileName(mOptions.outputFilePrefix, mMiningRun);
    string rulesOutputFilename =
      GetOutputRuleFileName(mOptions.outputFilePrefix, mMiningRun);
    MineFPTree(mTree,
               mOptions.minSup,
               itemSetsOuputFilename,
               rulesOutputFilename,
               mIndex,
               mOptions.treePruneDepth,
               mOptions.countItemSetsOnly,
               mOptions.countRulesOnly,
               GetItemFilter());
  }
}

void FPTreeFunctor::OnUnload(const std::vector<Item>& txn) {
}

void FPTreeFunctor::OnEndLoad() {
}
