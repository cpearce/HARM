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

// Comparison in non-increasing order of frequency, tie-breaking on appearnce
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


struct CountCmp {
  CountCmp(DataSet* i) : index(i) {}
  bool operator()(Item a, Item b) {
    return index->Count(a) > index->Count(b);
  }
  DataSet* index;
};

bool VerifySortedByCount(vector<Item>& txn, DataSet* index) {
  for (unsigned i = 1; i < txn.size(); i++) {
    int p = index->Count(txn[i - 1]);
    int t = index->Count(txn[i]);
    if (p < t) {
      return false;
    }
  }
  return true;
}


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
                              FPNode* tree,
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

void FPGrowth(FPNode* tree,
              PatternOutputStream& output,
              vector<Item>& pattern,
              DataSet* index,
              const double minCount,
              unsigned nodePruneDepth = std::numeric_limits<unsigned>::max(),
              ItemFilter* = nullptr);

void MineFPTree(FPNode* fptree,
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
  PatternOutputStream output(writeItemSets ? itemSetsOuputFilename.c_str() : 0,
                             writeItemSets ? index : 0);
  vector<Item> pattern;
  FPGrowth(fptree, output, pattern, index, minCount, treePruneDepth, filter);
  output.Close();

  Log("FPGrowth generated %lld patterns in %.3lfs%s\n",
      output.GetNumPatterns(), timer.Seconds(),
      (!writeItemSets ? " (not saved to disk)" : ""));

  #ifdef _DEBUG
  if (writeItemSets) {
    PatternInputStream input;
    input.Open(itemSetsOuputFilename.c_str());
    ASSERT(input.IsOpen());
    vector<ItemSet> v = input.ToVector();
    ASSERT(v.size() == output.GetNumPatterns());
  }
  #endif

  if (!writeItemSets) {
    Log("Skipping rule generation because itemsets weren't saved to disk to generate from\n");
  } else {
    long numRules = 0;
    PatternInputStream input;
    input.Open(itemSetsOuputFilename.c_str());
    ASSERT(input.IsOpen());
    GenerateRules(input, 0.9, 1.0, numRules, index, rulesOuputFilename, countRulesOnly);
  }
  Log("-----------------------------------------------\n");
}

void FPGrowth(FPNode* tree,
              PatternOutputStream& output,
              vector<Item>& pattern,
              DataSet* index,
              const double minCount,
              unsigned nodePruneDepth,
              ItemFilter* filter) {
  ASSERT(tree != 0);
  if (tree->HasSinglePath()) {
    FPNode* t = tree->IsRoot() ? tree->FirstChild() : tree;
    if (t) {
      AddPatternsInPath(t, output, pattern, nodePruneDepth, filter);
    }
  } else {
    // For each item in the nodelist, construct a new tree, minus the item.
    // Then recurse on the new tree and its new nodelist.
    ItemMap<FPNode*>::Iterator itr = tree->headerTable->GetIterator();
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
      FPNode* subtree = FPNode::CreateRoot();

      // Note: we don't pass the ItemFilter to ConstructConditionalTree, as we
      // assume that anything above |item| in the tree also will be signalled to
      // be kept by the filter.
      ConstructConditionalTree(node, subtree, minCount, nodePruneDepth);
      pattern.push_back(item);
      output.Write(pattern);
      if (!subtree->IsLeaf()) {
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
LoadFunctor* CreateLoadFunctor(DataSet* index, FPNode* fptree, Options& options) {
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

void ConstructFPInitialTree(FPNode* fptree,
                            DataSet* index,
                            Options& options) {
  assert(options.mode == kFPTree);
  assert(index->IsLoaded());

  DataSetReader reader;
  if (!reader.Open(options.inputFileName)) {
    cerr << "ERROR: Can't open " << options.inputFileName << " failing!" << endl;
    exit(-1);
  }

  TreeMetricsLogger logger(fptree, options.logTreeTxn);
  TransactionId tid = 0;
  while (true) {
    Transaction transaction(tid);
    if (!reader.GetNext(transaction.items)) {
      // Failed to read, no more transactions.
      break;
    }
    // Sort in non-increasing order by count.
    CountCmp c(index);
    sort(transaction.items.begin(), transaction.items.end(), c);
    ASSERT(VerifySortedByCount(transaction.items, index));

    fptree->Insert(transaction.items);
    logger.OnTxn();

    // Increment the transaction id, so that the next transaction
    // has a monotonically increasing id.
    tid++;
  }
}

FPNode* CreateFPTree(DataSet* aDataSet, Options& options) {
  FPNode* fptree = FPNode::CreateRoot();
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
  if (isStreaming) {
    index = new WindowIndex(options.inputFileName.c_str(), NULL, options.blockSize);
  } else {
    index = new InvertedDataSetIndex(options.inputFileName.c_str());
  }
  FPNode* fptree = CreateFPTree(index, options);
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


TreeMetricsLogger::TreeMetricsLogger(FPNode* _tree,
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

FPTreeFunctor::FPTreeFunctor(FPNode* aTree,
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

void FPTreeFunctor::OnStartLoad() {
}

void FPTreeFunctor::OnLoad(const std::vector<Item>& txn) {
  mLogger.OnTxn();
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
  if (mOptions.mode == kFPTree) {
    // Vanilla FPTree mode constructs its FPTree after loading the dataset,
    // so that it knows which items are frequent.
    ConstructFPInitialTree(mTree, mIndex, mOptions);
  }
}

static string GetPath(FPNode* n) {
  string path;
  while (!n->IsRoot()) {
    string s = n->item;
    char buf[32];
    sprintf(buf, "%s:%u%s", s.c_str(), n->count, (n->parent->IsRoot() ? "" : " "));
    path.append(buf);
    n = n->parent;
  }
  return path;
}

/*
i1,i2,i5
i2,i4
i2,i3
i1,i2,i4
i1,i3
i2,i3
i1,i3
i1,i2,i3,i5
i1,i2,i3
*/
void TestInitialConstruction() {
  InvertedDataSetIndex index("datasets/test/fp-test.csv");
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  FPNode* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string s = fptree->ToString();
  //cout << s.c_str() << endl;
  ASSERT(s == "(root:0 (i1:2 (i3:2)) (i2:7 (i1:4 (i3:2 (i5:1)) (i4:1) (i5:1)) (i3:2) (i4:1)))");
  ASSERT(fptree->NumNodes() == 11);

  PatternOutputStream output("datasets/test/fp-test-patterns.csv", 0);
  vector<Item> pattern;
  FPGrowth(fptree, output, pattern, &index, 0);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/fp-test-patterns.csv");
  ASSERT(input.IsOpen());
  cout << "Results:\n";
  ItemSet itemset;
  while (!(itemset = input.Read()).IsNull()) {
    string s = itemset;
    cout << s.c_str() << endl;
  }
  ASSERT(!fptree->HasSinglePath());

  // Test the headerTable was maintained during insertion.
  // Check that i5 has two paths, and that they're correct.
  /*FPNode* n = fptree->headerTable->find(Item("i5"))->second;*/
  FPNode* n = fptree->headerTable->Get(Item("i5"));

  string path = GetPath(n);
  cout << path.c_str() << endl;
  ASSERT(path == "i5:1 i3:2 i1:4 i2:7");

  n = n->next;
  ASSERT(n && !n->next); // Should have one more path for i5.
  path = GetPath(n);
  ASSERT(path == "i5:1 i1:4 i2:7");

  delete fptree;
}


void TestHasSinglePath() {
  InvertedDataSetIndex index("datasets/test/single-path.csv");
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  FPNode* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string s = fptree->ToString();
  cout << s.c_str() << endl;
  ASSERT(s == "(root:0 (i1:2 (i2:2 (i3:2))))");
  ASSERT(fptree->NumNodes() == 4);

  ASSERT(fptree->HasSinglePath());
}

template<class T>
string Flatten(const vector<T>& v) {
  string s("(");
  for (unsigned i = 0; i < v.size(); i++) {
    string item = v[i];
    s.append(item);
    if (i + 1 != v.size()) {
      s.append(",");
    }
  }
  s.append(")");
  return s;
}

void TestAddPatternsInPath() {

  FPNode* t = FPNode::CreateRoot();
  vector<Item> itemset;
  itemset.push_back(Item("i1"));
  itemset.push_back(Item("i2"));
  itemset.push_back(Item("i3"));
  itemset.push_back(Item("i4"));
  t->Insert(itemset);

  string s = t->ToString();
  cout << s.c_str() << endl;

  PatternOutputStream output("datasets/test/add-patterns-in-path-test.csv", 0);
  vector<Item> path;
  AddPatternsInPath(t->FirstChild(), output, path);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/add-patterns-in-path-test.csv");
  ASSERT(input.IsOpen());
  vector<ItemSet> v = input.ToVector();
  cout << "Results:\n";
  s = Flatten(v);
  ASSERT(s == "(i1,i2,i3,i4,i3 i4,i2 i3,i2 i4,i2 i3 i4,i1 i2,i1 i3,i1 i4,i1 i3 i4,i1 i2 i3,i1 i2 i4,i1 i2 i3 i4)");

}

void TestConstructConditionalTree_inner(DataSet* index,
                                        FPNode* condTree,
                                        const char* item,
                                        const char* res_conf0,
                                        const unsigned min_count,
                                        const char* res_conf_count) {
  FPNode* headerList = condTree->headerTable->Get(Item(item));
  ASSERT(headerList != 0);

  {
    FPNode* tree = FPNode::CreateRoot();
    ConstructConditionalTree(headerList, tree, 0);
    string s = tree->ToString();
    ASSERT(s == res_conf0);
    delete tree;
  }
  {
    FPNode* tree = FPNode::CreateRoot();
    ConstructConditionalTree(headerList, tree, min_count);
    string s = tree->ToString();
    ASSERT(s == res_conf_count);
    delete tree;
  }
}

void TestConstructConditionalTree() {
  {
    InvertedDataSetIndex index("datasets/test/fp-test.csv");
    Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
    FPNode* fptree = CreateFPTree(&index, options);
    if (!fptree) {
      return;
    }
    index.Load();

    cout << "Tree: " << fptree->ToString() << endl;

    TestConstructConditionalTree_inner(&index, fptree, "i5", "(root:0 (i2:2 (i1:2 (i3:1))))", 2, "(root:0 (i2:2 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i4", "(root:0 (i2:2 (i1:1)))", 2, "(root:0 (i2:2))");
    TestConstructConditionalTree_inner(&index, fptree, "i3", "(root:0 (i1:2) (i2:4 (i1:2)))", 3, "(root:0 (i1:2) (i2:4 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i3", "(root:0 (i1:2) (i2:4 (i1:2)))", 2, "(root:0 (i1:2) (i2:4 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i2", "(root:0)", 2, "(root:0)");
    TestConstructConditionalTree_inner(&index, fptree, "i1", "(root:0 (i2:4))", 5, "(root:0)");

    delete fptree;
  }
  {
    InvertedDataSetIndex index("datasets/test/fp-test5.csv");
    Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
    FPNode* fptree = CreateFPTree(&index, options);
    if (!fptree) {
      return;
    }
    index.Load();

    cout << "Tree: " << fptree->ToString() << endl;
    TestConstructConditionalTree_inner(&index, fptree, "f=1", "(root:0 (e=1:2 (b=0:1)))", 2, "(root:0 (e=1:2))");
  }
}

static void TestFPGrowth() {
  InvertedDataSetIndex index("datasets/test/fp-test.csv");
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  FPNode* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string ts = fptree->ToString();
  cout << "Tree: " << ts << endl;

  PatternOutputStream output("datasets/test/fp-test-itemsets.csv", 0);
  vector<Item> vec;
  FPGrowth(fptree, output, vec, &index, 2);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/fp-test-itemsets.csv");
  vector<ItemSet> v = input.ToVector();

  cout << "TestFPGrowth() generated rules (" << v.size() << "):" << endl;
  for (unsigned i = 0; i < v.size(); ++i) {
    cout << ((string)v[i]).c_str() << endl;
  }
  cout << Flatten(v).c_str() << endl;

  delete fptree;
}

static void TestFPGrowth2() {
  cout << endl << "test fp3" << endl;
  // -i datasets/test/fp-test3.csv -m fptree -o output/fptree-fp-test3 -minsup 0.2
  InvertedDataSetIndex index("datasets/test/fp-test3.csv");
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  FPNode* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string ts = fptree->ToString();
  cout << "Tree: " << ts << endl;

  PatternOutputStream output("datasets/test/fptree-fp-test-3-itemsets.csv", 0);
  double minCount = index.NumTransactions() * 0.2;
  vector<Item> vec;
  FPGrowth(fptree, output, vec, &index, minCount);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/fptree-fp-test-3-itemsets.csv");
  vector<ItemSet> v = input.ToVector();

  cout << "TestFPGrowth() generated rules (" << v.size() << "):" << endl;
  for (unsigned i = 0; i < v.size(); ++i) {
    cout << ((string)v[i]).c_str() << endl;
  }
  cout << Flatten(v).c_str() << endl;

  delete fptree;
}

static void TestStream() {
  {

    Options options(0, kStream, UINT_MAX, 0, 0, 0, 0, 0, 5);
    options.inputFileName = "datasets/test/census2.csv";
    options.outputFilePrefix = "census2-out";

    InvertedDataSetIndex index("datasets/test/census2.csv");
    FPNode* cantree = CreateFPTree(&index, options);
    if (!cantree) {
      return;
    }
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    cantree->DumpToGraphViz("test-output/cptree-census2.dot");

    printf("freq before sort:\n");
    cantree->DumpFreq();
    cantree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    cantree->DumpFreq();

    cantree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    ASSERT(cantree->IsSorted());

    delete cantree;
  }
}


static void TestTreeSorted() {
  {
    InvertedDataSetIndex index("datasets/test/census2.csv");
    Options options(0, kFPTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    FPNode* cantree = CreateFPTree(&index, options);
    if (!cantree) {
      return;
    }
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    cantree->DumpToGraphViz("datasets/test/census.dot");

    cantree->Sort();
    ASSERT(cantree->IsSorted());

    delete cantree;
  }

  {
    vector<Item> v;
    for (unsigned i = 0; i < 4; i++) {
      char buf[4];
      sprintf(buf, "%c", (char)('a' + i));
      v.push_back(Item(buf));
    }
    string s = Flatten(v);
    ASSERT(s == "(a,b,c,d)");
    std::reverse(v.begin(), v.end());
    s = Flatten(v);
    ASSERT(s == "(d,c,b,a)");
  }

  {
    Item::ResetBaseId();

    InvertedDataSetIndex index("datasets/test/fp-test3.csv");
    Options options(0, kCpTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    FPNode* cantree = CreateFPTree(&index, options);
    if (!cantree) {
      return;
    }
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    cantree->DumpToGraphViz("test-output/cp-tree-fp-test3.dot");

    ASSERT(cantree->IsSorted());

    cantree->Sort();
    cantree->DumpToGraphViz("test-output/cp-tree-fp-test3.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    ASSERT(cantree->IsSorted());

    delete cantree;
  }
  {
    Item::ResetBaseId();

    InvertedDataSetIndex index("datasets/test/census2.csv");
    Options options(0, kCpTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    FPNode* cantree = CreateFPTree(&index, options);
    if (!cantree) {
      return;
    }
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    cantree->DumpToGraphViz("test-output/cptree-census2.dot");

    printf("freq before sort:\n");
    cantree->DumpFreq();
    cantree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    cantree->DumpFreq();

    cantree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    ASSERT(cantree->IsSorted());

    delete cantree;
  }
}

void TestSpoTree() {
  {
    ItemMap<unsigned> m;
    for (unsigned i = 1; i < 100; i++) {
      Item item(i);
      ASSERT(!m.Contains(item));
      m.Set(item, i);
      ASSERT(m.Contains(item));
      ASSERT(m.Get(item) == i);
    }
  }
  {
    Item::ResetBaseId();

    InvertedDataSetIndex index("datasets/test/census2.csv");
    Options options(0, kSpoTree, 0, 0, 0.15, 0, 0, 0, 0);
    FPNode* spotree = CreateFPTree(&index, options);
    if (!spotree) {
      return;
    }
    index.Load();

    string ts = spotree ->ToString();
    cout << "Tree: " << ts << endl;
    spotree->DumpToGraphViz("test-output/spotree-census2.dot");

    ASSERT(spotree->IsSorted());
    /*
    printf("freq before sort:\n");
    spotree->DumpFreq();
    spotree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    spotree->DumpFreq();

    spotree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = spotree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    ASSERT(spotree->IsSorted());*/

    delete spotree;
  }


}

void Test_FPTree() {
  TestInitialConstruction();
  TestHasSinglePath();
  TestAddPatternsInPath();
  TestConstructConditionalTree();
  TestFPGrowth();
  TestFPGrowth2();
  TestTreeSorted();
  TestSpoTree();
  TestStream();
}
