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

#include <iostream>
#include <memory>
#include <map>

#include "Item.h"
#include "List.h"
#include "ItemMap.h"
#include "utils.h"


class FPNode;


struct ItemComparator {
  virtual bool operator()(const Item a, const Item b) = 0;
};

//Contain node data use for comparison Extrap vs Cp
struct CmpNode {
  int nId;
  int nDepth;
  int nCount;

  void display() {
    std::cout << "(ID" << nId << ",D" << nDepth << ",C" << nCount << ") ";
  }
  void Write() {
    Log("(ID %d, D %d, C %d)", nId, nDepth, nCount) ;
  }
};

// Compares two items based on the frequencies specified in the item frequency
// table passed in.
template<typename FreqType>
struct ItemMapCmp : public ItemComparator {
  ItemMapCmp(const ItemMap<FreqType>& f) : freq(f) {}
  bool operator()(const Item a, const Item b) override {
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
  const ItemMap<FreqType>& freq;
};

class FPTree;

class FPNode {
  friend class FPTree;
public:

  Item item;
  unsigned count;
  std::map<Item, FPNode*> children;

  // Next item in the header table list of nodes.
  FPNode* next;

  // Previous item in the header table list of nodes.
  FPNode* prev;

  // Parent node. Root has a null parent.
  FPNode* parent;

  // Number of parent nodes. Root is depth 0.
  const unsigned depth;

  // Our token in the leaf list. This enables us to delete the node from
  // the leaf list in O(1) time.
  List<FPNode*>::Token leafToken;

  void DumpFreq() const;

  ~FPNode();

  bool IsRoot() const {
    bool isRoot = item.IsNull();
    ASSERT(isRoot == (depth == 0));
    return isRoot;
  }

  unsigned NumNodes() const;

  bool IsLeaf() const {
    bool rv = children.empty();
    ASSERT(IsRoot() || (rv == leafToken.IsInList()));
    return rv;
  }

  bool IsSorted() const;

  FPNode* FirstChild() const {
    return children.size() > 0 ? children.begin()->second : 0;
  }

  void Sort();
  void Sort(ItemComparator* cmp);

  // Sorts a transaction based on the existing frequency table (iList) if
  // we have one, otherwise it's sorted in order of item appearance.
  void SortTransaction(std::vector<Item>& txn);

  void Insert(const std::vector<Item>& txn) {
    Insert(txn, 1);
  }

  void Insert(const std::vector<Item>& txn, unsigned count) {
    Insert(txn.begin(), txn.end(), count);
  }

  std::string ToString() const;
  std::string ToString(uint32_t stopDepth) const;
  bool ToVector(std::vector<int32_t>* v) const;
  bool ToVector(uint32_t stopDepth, std::vector<int32_t>* v) const;
  void ToFile(char* fp);
  void ToFile(char* fp, uint32_t stopDepth);
  //CmpNode
  bool ToVector(std::vector<CmpNode>* v) const;
  bool ToVector(uint32_t stopDepth, std::vector<CmpNode>* v) const;

  void DumpToGraphViz(const char* filename) const;

  void Remove(const std::vector<Item>& path);

  ItemMap<FPNode*>& HeaderTable() const;
  ItemMap<unsigned>& FrequencyTable() const;
  ItemMap<unsigned>& FrequencyTableAtLastSort() const;
  List<FPNode*>& Leaves() const;

private:

  bool HasSinglePath() const {
    if ((!parent || (parent->IsRoot() && parent->children.size() == 1)) && children.size() == 0) {
      return false;
    }
    if (children.size() == 0) {
      return true;
    }
    else if (children.size() == 1) {
      return FirstChild()->HasSinglePath();
    }
    else {
      return false;
    }
  }

  void Decrement(uint32_t aCount);

  void Remove(std::vector<Item>::const_iterator begin,
              std::vector<Item>::const_iterator end,
              unsigned count);

  FPNode* GetOrCreateChild(Item aItem);
  FPNode* GetChild(Item aItem) const;

  void Replace(const std::vector<Item>& from,
               const std::vector<Item>& to,
               uint32_t count);

  void DumpToGraphViz(FILE* f, std::string p) const;

  bool DoIsSorted() const;

  // Creates a root node.
  explicit FPNode(FPTree* aTree)
    : count(0)
    , next(nullptr)
    , prev(nullptr)
    , parent(nullptr)
    , depth(0)
    , mTree(aTree)
  {
  }

  // Creates a data node.
  explicit FPNode(FPTree* aTree,
                  Item aItem,
                  FPNode* aParent,
                  unsigned aCount,
                  unsigned aDepth)
    : item(aItem)
    , count(aCount)
    , next(nullptr)
    , prev(nullptr)
    , parent(aParent)
    , depth(aDepth)
    , mTree(aTree)
  {
    // All nodes initially have no children, so are leaves to start with.
    leafToken = Leaves().Prepend(this);
  }

  // Inserts the items in txn into the tree. Increments the support count by
  // |count|. idx is the index into txn which we're currently adding, idx+1
  // is added to this node's children, and so on.
  void Insert(std::vector<Item>::const_iterator begin,
              std::vector<Item>::const_iterator end,
              unsigned count);

  void AddToHeaderTable(FPNode* n);

  FPTree* mTree;
};

class FPTree {
public:

  FPTree()
    : mRoot(new FPNode(this))
  {
  }

  ~FPTree()
  {
    // Note: We must explicitly delete this here so that the destruction
    // of the tree is performed before the other fields on the tree are
    // destroyed, as the FPNode's destructors use data stored on the FPTree.
    mRoot = nullptr;
  }

  FPNode* GetRoot() const { return mRoot; }
  ItemMap<FPNode*>& HeaderTable() { return mHeaderTable; }
  ItemMap<unsigned>& FrequencyTable() { return mFreq; }
  ItemMap<unsigned>& FrequencyTableAtLastSort() { return mFreqAtLastSort; }
  List<FPNode*>& Leaves() { return mLeaves; }

  bool HasSinglePath() const {
    return mRoot->HasSinglePath();
  }

  bool IsEmpty() const {
    return mRoot->IsLeaf();
  }

  void Insert(const std::vector<Item>& txn, unsigned count) {
    mRoot->Insert(txn.begin(), txn.end(), count);
  }
  void Insert(const std::vector<Item>& txn) {
    mRoot->Insert(txn);
  }

  void Remove(const std::vector<Item>& path) {
    mRoot->Remove(path);
  }

  std::string ToString() const {
    return mRoot->ToString();
  }

  // Sorts a transaction based on the existing frequency table (iList) if
  // we have one, otherwise it's sorted in order of item appearance.
  void SortTransaction(std::vector<Item>& txn) {
    mRoot->SortTransaction(txn);
  }

  void Sort() {
    mRoot->Sort();
  }

  void Sort(ItemComparator* cmp) {
    mRoot->Sort(cmp);
  }

  unsigned NumNodes() const {
    return mRoot->NumNodes();
  }

  bool ToVector(std::vector<int32_t>* v) const {
    return mRoot->ToVector(v);
  }

  bool ToVector(int stopDepth, std::vector<int32_t>* v) const {
    return mRoot->ToVector(stopDepth, v);
  }

  //CmpNode
  bool ToVector(std::vector<CmpNode>* v) const {
    return mRoot->ToVector(v);
  }

  bool ToVector(int stopDepth, std::vector<CmpNode>* v) const {
    return mRoot->ToVector(stopDepth, v);
  }

  void DumpToGraphViz(const char* filename) const {
    mRoot->DumpToGraphViz(filename);
  }

  void DumpFreq() const {
    mRoot->DumpFreq();
  }

  bool IsSorted() const {
    return mRoot->IsSorted();
  }

private:
  AutoPtr<FPNode> mRoot;
  ItemMap<FPNode*> mHeaderTable;
  List<FPNode*> mLeaves;

  // Current frequency table. This is updated as we add items to the tree.
  ItemMap<unsigned> mFreq;

  // Frequency table at the last sort. This is only updated when we sort.
  // We keep this separate from the current frequency table in |freq| so that
  // we sort consistently during insertion.
  ItemMap<unsigned> mFreqAtLastSort; // iList;
};

class TreePathIterator {
public:
  TreePathIterator(FPTree* tree);

  // Stores the next path into |path|, and the |count| of the corresponding
  // leaf into |count|. The count is the number of times the itemset appears
  // in the dataset. Returns true on success, or false if all paths have
  // been iterated over.
  bool GetNext(std::vector<Item>& path, unsigned& count);

private:
  FPNode* root;
  std::unique_ptr<List<FPNode*>::Iterator> itr;
};
