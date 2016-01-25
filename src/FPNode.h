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

class FPNode {
public:

  Item item;
  unsigned count;
  std::map<Item, FPNode*> children;
  ItemMap<FPNode*>* headerTable;
  List<FPNode*>* leaves;

  // Current frequency table. This is updated as we add items to the tree.
  ItemMap<unsigned>* freq;

  // Frequency table at the last sort. This is only updated when we sort.
  // We keep this separate from the current frequency table in |freq| so that
  // we sort consistently during insertion.
  ItemMap<unsigned>* iList;

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

  static FPNode* CreateRoot() {
    FPNode* n = new FPNode();
    return n;
  }

  ~FPNode();

  bool IsRoot() const {
    bool isRoot = item.IsNull();
    ASSERT(isRoot == (depth == 0));
    return isRoot;
  }

  unsigned NumNodes() const;

  bool IsLeaf() const {
    return children.size() == 0;
  }

  bool HasSinglePath() const {
    if ((!parent || (parent->IsRoot() && parent->children.size() == 1)) && children.size() == 0) {
      return false;
    }
    if (children.size() == 0) {
      return true;
    } else if (children.size() == 1) {
      return FirstChild()->HasSinglePath();
    } else {
      return false;
    }
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
  std::string ToString(int stopDepth) const;
  bool ToVector(std::vector<int32_t>* v) const;
  bool ToVector(int stopDepth, std::vector<int32_t>* v) const;
  void ToFile(char* fp);
  void ToFile(char* fp, int  stopDepth);
  bool ToVector(std::vector<std::pair<int, int>>* v) const;
  bool ToVector(int stopDepth, std::vector<std::pair<int, int>>* v) const;
  bool ToVector(std::vector<std::pair<int, std::pair<int, int>>>* v) const;
  bool ToVector(int stopDepth, std::vector<std::pair<int, std::pair<int, int>>>* v) const;
  //CmpNode
  bool ToVector(std::vector<CmpNode>* v) const;
  bool ToVector(int stopDepth, std::vector<CmpNode>* v) const;

  void DumpToGraphViz(const char* filename) const;

  void Remove(const std::vector<Item>& path);

private:

  void Decrement(uint32_t aCount);

  void Remove(std::vector<Item>::const_iterator begin,
              std::vector<Item>::const_iterator end,
              unsigned count);

  FPNode* FPNode::GetOrCreateChild(Item aItem);
  FPNode* FPNode::GetChild(Item aItem) const;

  void Replace(const std::vector<Item>& from,
               const std::vector<Item>& to,
               uint32_t count);

  void DumpToGraphViz(FILE* f, std::string p) const;

  bool DoIsSorted() const;

  // Creates a root node.
  FPNode()
    : count(0),
      iList(0),
      next(0),
      prev(0),
      parent(0),
      depth(0) {
    headerTable = new ItemMap<FPNode*>();
    leaves = new List<FPNode*>();
    freq = new ItemMap<unsigned>();
    if (!headerTable || !freq) {
      std::cout << "Out of memory" << std::endl;
      exit(-1);
    }
  }

  // Creates a data node.
  FPNode(Item i,
         FPNode* p,
         ItemMap<FPNode*>* l,
         ItemMap<unsigned>* f,
         List<FPNode*>* lvs,
         unsigned c,
         ItemMap<unsigned>* ilist,
         unsigned _depth)
    : item(i),
      count(c),
      headerTable(l),
      leaves(lvs),
      freq(f),
      iList(ilist),
      next(0),
      prev(0),
      parent(p),
      depth(_depth) {
    // All nodes initially have no children, so are leaves to start with.
    leafToken = leaves->Prepend(this);
  }

  // Inserts the items in txn into the tree. Increments the support count by
  // |count|. idx is the index into txn which we're currently adding, idx+1
  // is added to this node's children, and so on.
  void Insert(std::vector<Item>::const_iterator begin,
              std::vector<Item>::const_iterator end,
              unsigned count);

  void AddToHeaderTable(FPNode* n);
};

class TreePathIterator {
public:
  TreePathIterator(FPNode* root);

  // Stores the next path into |path|, and the |count| of the corresponding
  // leaf into |count|. The count is the number of times the itemset appears
  // in the dataset. Returns true on success, or false if all paths have
  // been iterated over.
  bool GetNext(std::vector<Item>& path, unsigned& count);

private:
  FPNode* root;
  std::unique_ptr<List<FPNode*>::Iterator> itr;
};
