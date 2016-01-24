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

#include "FPNode.h"
#include "FPTree.h"
#include <algorithm>
#include <stdint.h>

using namespace std;

FPNode::~FPNode() {
  if (!IsRoot()) {
    // This is a regular node, it's also appears in the linked list in
    // the header table. Remove node from the header table.
    if (!prev) {
      // This node is the first entry in header table.
      if (next) {
        // We have a next, set the first entry to that.
        headerTable->Set(item, next);
        next->prev = nullptr;
      } else {
        // No next, just erase the entry for this item.
        headerTable->Erase(item);
      }
    } else {
      if (next) {
        next->prev = prev;
      }
      prev->next = next;
    }
    if (IsLeaf()) {
      ASSERT(leafToken.IsInList());
      leaves->Erase(leafToken);
      ASSERT(!leafToken.IsInList());
    }
  }
  // Delete our child trees.
  map<Item, FPNode*>::iterator itr = children.begin();
  while (itr != children.end()) {
    delete itr->second;
    itr++;
  }
  children.clear();
  if (IsRoot()) {
    // The root owns the header table and other shared data.
    // We must delete them if we're the root.
    delete headerTable;
    delete leaves;
    delete freq;
    delete iList;
  }
  headerTable = nullptr;
  freq = nullptr;
  leaves = nullptr;
}

string FPNode::ToString() const {
  string s("(");
  s.append(item.IsNull() ? "root" : item);
  s.append(":");
  s.append(std::to_string(count));

  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    s.append(" ");
    s.append(itr->second->ToString());
    itr++;
  }
  s.append(")");
  return s;
}

string FPNode::ToString(int stopDepth) const {
  if (depth >= stopDepth) {
    return string("");
  }
  string s("(");
  s.append(item.IsNull() ? "root" : item);
  s.append(":");
  s.append(std::to_string(count));

  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    s.append(" ");
    s.append(itr->second->ToString());
    itr++;
  }
  s.append(")");
  return s;
}

void FPNode::ToFile(char* fn) {
  ofstream fp(fn);
  string s = this->ToString();
  if (fp.is_open()) {
    fp << s;
  } else {
    cerr << "FPNode::ToFile File is not open.\n";
  }
}

void FPNode::ToFile(char* fn, int  stopDepth) {
  ofstream fp(fn);
  string s = this->ToString(stopDepth);
  if (fp.is_open()) {
    fp << s;
  } else {
    cerr << "FPNode::ToFile File is not open.\n";
  }
}
bool FPNode::ToVector(vector<int32_t>* v) const {
  v->push_back(item.IsNull() ? 0 : item.GetId());
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(v);
    itr++;
  }
  return true;
}

bool FPNode::ToVector(int stopDepth, vector<int32_t>* v) const {
  if (depth >= stopDepth) {
    return false;
  }
  v->push_back(item.IsNull() ? 0 : item.GetId());
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(stopDepth, v);
    itr++;
  }
  return true;
}

// Read both node and depth to vector
bool FPNode::ToVector(vector<pair<int, int>>* v) const {
  pair<int, int> p;
  p.first = item.IsNull() ? -1 : item.GetId();
  p.second = depth;
  v->push_back(p);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(v);
    itr++;
  }
  return true;
}

bool FPNode::ToVector(int stopDepth, vector<pair<int, int>>* v) const {
  if (depth >= stopDepth) {
    return false;
  }
  pair<int, int> p;
  p.first = item.IsNull() ? -1 : item.GetId();
  p.second = depth;
  v->push_back(p);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(stopDepth, v);
    itr++;
  }
  return true;
}

// Read node id, depth and frequency
bool FPNode::ToVector(vector<pair<int, pair<int, int>>>* v) const {
  pair<int, pair<int, int>> p;
  p.first = item.IsNull() ? -1 : item.GetId();
  p.second.first  = depth;
  p.second.second = count;
  v->push_back(p);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(v);
    itr++;
  }
  return true;
}

bool FPNode::ToVector(int stopDepth, vector<pair<int, pair<int, int>>>* v) const {
  if (depth >= stopDepth) {
    return false;
  }
  pair<int, pair<int, int>> p;
  p.first = item.IsNull() ? -1 : item.GetId();
  p.second.first  = depth;
  p.second.second = count;
  v->push_back(p);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(stopDepth, v);
    itr++;
  }
  return true;
}

bool FPNode::ToVector(vector<CmpNode>* v) const {
  CmpNode n;
  n.nId = item.IsNull() ? -1 : item.GetId();
  n.nDepth = depth;
  n.nCount = count;
  v->push_back(n);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(v);
    itr++;
  }
  return true;
}

bool FPNode::ToVector(int stopDepth, vector<CmpNode>* v) const {
  if (depth >= stopDepth) {
    return false;
  }
  CmpNode  p;
  p.nId = item.IsNull() ? -1 : item.GetId();
  p.nDepth = depth;
  p.nCount = count;
  v->push_back(p);
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    itr->second->ToVector(stopDepth, v);
    itr++;
  }
  return true;
}

unsigned FPNode::NumNodes() const {
  unsigned n = 1; // for this node...
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    FPNode* node = itr->second;
    n += node->NumNodes();
    itr++;
  }
  return n;
}

void FPNode::DumpToGraphViz(const char* filename) const {
  FILE* f = fopen(filename, "w");
  ASSERT(f);
  fprintf(f, "digraph G {\n");
  fprintf(f, "\troot;\n");
  DumpToGraphViz(f, "root");
  fprintf(f, "}\n");
  fclose(f);
}

void FPNode::DumpToGraphViz(FILE* f, string parent) const {
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    FPNode* child = itr->second;
    string name = (string)child->item;
    string nodeName = parent + "_" + name;

    fprintf(f, "\t%s [label=\"%s:%d\"];\n", nodeName.c_str(), name.c_str(), child->count);

    fprintf(f, "\t%s -> %s;\n", parent.c_str(), nodeName.c_str());
    child->DumpToGraphViz(f, nodeName);
    itr++;
  }
}

bool VerifySortedByFreqAppearance(const vector<Item>& v,
                                  const ItemMap<unsigned>* freq) {
  for (unsigned i = 1; i < v.size(); i++) {
    Item prev = v[i - 1];
    Item curr = v[i];

    unsigned curCount = freq->Get(curr, 0);
    unsigned prevCount = freq->Get(prev, 0);
    if (prevCount < curCount) {
      return false;
    }
    if (prevCount == curCount &&
        v[i - 1].GetId() > v[i].GetId()) {
      return false;
    }
  }
  return true;
}

void FPNode::SortTransaction(vector<Item>& transaction) {
  if (iList) {
    ItemMapCmp<unsigned> cmp(*iList);
    sort(transaction.begin(), transaction.end(), cmp);
    ASSERT(VerifySortedByFreqAppearance(transaction, iList));
  } else {
    AppearanceCmp cmp;
    sort(transaction.begin(), transaction.end(), cmp);
    ASSERT(VerifySortedByAppearance(transaction));
  }
}

void FPNode::Insert(const vector<Item>& txn,
                    unsigned idx,
                    unsigned count) {
  ASSERT(idx < txn.size());

  Item item = txn[idx];
  map<Item, FPNode*>::iterator itr = children.find(item);
  FPNode* node = (itr != children.end()) ? itr->second : nullptr;
  if (!node) {
    // Item is not in child list, create a new node for it.
    node = new FPNode(item, this, headerTable, freq, leaves, count, iList, depth + 1);

    if (!IsRoot() && IsLeaf()) {
      // We're about to add a child node, so we'll stop being a leaf.
      // Remove us from the leaves list.
      ASSERT(leafToken.IsInList());
      leaves->Erase(leafToken);
      ASSERT(!leafToken.IsInList());
    }
    children[item] = node;
    ASSERT(!IsLeaf());

    // Add new item to the headerTable.
    AddToHeaderTable(node);
  } else {
    // Item already in child list, increment its count.
    node->count += count;
  }
  freq->Set(item, freq->Get(item, 0) + count);

  if (idx + 1 < txn.size()) {
    // Recurse onto the child nodes.
    node->Insert(txn, idx + 1, count);
  }

  ASSERT(!IsLeaf() || leafToken.IsInList());

}

void FPNode::Remove(const vector<Item>& path, unsigned idx, unsigned count) {
  ASSERT(idx < path.size());
  ASSERT(IsRoot() || this->item == path[idx - 1]);

  Item item = path[idx];
  FPNode* node = children[item];
  ASSERT(node);

  ASSERT(node->count >= count);
  node->count -= count;
  ASSERT(freq->Get(item) >= count);
  ASSERT(freq->Contains(item));
  freq->Set(item, freq->Get(item) - count);

  if (idx + 1 < path.size()) {
    node->Remove(path, idx + 1, count);
  }

  ASSERT(!node->IsLeaf() || node->leafToken.IsInList());

  if (node->count == 0) {
    ASSERT(node->children.size() == 0);
    children.erase(item);
    ASSERT(this == node->parent);

    delete node;
    node = nullptr;
    if (!IsRoot() && count > 0 && IsLeaf()) {
      ASSERT(!leafToken.IsInList());
      leafToken = leaves->Append(this);
    }
  }

  // Note header table and leaves-list fixed up by FPNode destructor.
  ASSERT(!IsLeaf() || leafToken.IsInList());
}

static bool NotInList(const FPNode* node, const FPNode* list) {
  const FPNode* next = list;
  while (true) {
    if (!node || !next) {
      return true;
    }
    if (node == next) {
      return false;
    }
    next = next->next;
  }
}

void FPNode::AddToHeaderTable(FPNode* node) {
  if (headerTable->Contains(node->item)) {
    FPNode* list = headerTable->Get(node->item);
    node->next = list;
    ASSERT(!list->prev);
    list->prev = node;
  } else {
    ASSERT(!node->prev);
    ASSERT(!node->next);
  }
  headerTable->Set(node->item, node);
}


bool FPNode::DoIsSorted() const {
  ASSERT(!IsRoot());
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    Item childItem = itr->first;
    unsigned f = freq->Get(item, 0);
    unsigned cf = freq->Get(childItem, 0);
    if (cf == f && item.GetId() > childItem.GetId()) {
      return false;
    }
    if (cf > f) {
      return false;
    }
    FPNode* childNode = itr->second;
    if (!childNode->DoIsSorted()) {
      return false;
    }
    itr++;
  }
  return true;
}

bool FPNode::IsSorted() const {
  if (IsRoot()) {
    map<Item, FPNode*>::const_iterator itr = children.begin();
    while (itr != children.end()) {
      Item item = itr->first;
      FPNode* child = itr->second;
      if (!child->DoIsSorted()) {
        return false;
      }
      itr++;
    }
    return true;
  } else {
    return DoIsSorted();
  }
}


void FPNode::DumpFreq() const {
  ItemMap<unsigned>::Iterator itr = freq->GetIterator();
  while (itr.HasNext()) {
    Item item = itr.GetKey();
    unsigned count = itr.GetValue();
    printf("%s %u\n", ((string)item).c_str(), count);
    itr.Next();
  }
}

TreePathIterator::TreePathIterator(FPNode* _root)
  : root(_root),
    itr(root->leaves->Begin()) {
  ASSERT(root->IsRoot());
}

bool TreePathIterator::GetNext(std::vector<Item>& path, unsigned& count) {
  path.clear();
  if (!itr->HasNext()) {
    return false;
  }
  FPNode* leaf = itr->Next();
  ASSERT(leaf);
  ASSERT(leaf->IsLeaf());
  count = leaf->count;

  // Construct the path back to the root.
  uint32_t depth = leaf->depth;
  path.resize(depth);
  FPNode* n = leaf;
  uint32_t pos = depth - 1;
  while (!n->IsRoot()) {
    path[pos] = n->item;
    pos--;
    n = n->parent;
    ASSERT(pos < path.size() || n->IsRoot());
  }

  return true;
}

void FPNode::Sort() {
  // Only call on the root!
  ASSERT(IsRoot());

  if (iList) {
    delete iList;
  }
  iList = new ItemMap<unsigned>(*freq);

  ItemMapCmp<unsigned> cmp(*iList);
  Sort(&cmp);
}

void FPNode::Sort(ItemComparator* cmp) {
  DurationTimer timer;

  // Only call on the root!
  ASSERT(IsRoot());

  // We can't pass a virtual function pointer to std::sort. :(
  struct ConcreteItemComparator {
    ConcreteItemComparator(ItemComparator* comparator)
      : mCmp(comparator) {}
    bool operator()(const Item a, const Item b) {
      return (*mCmp)(a, b);
    }
    ItemComparator* mCmp;
  };
  ConcreteItemComparator comparator(cmp);

  TreePathIterator itr(this);
  vector<Item> path;
  unsigned count;
  while (itr.GetNext(path, count)) {
    // Sort path as per comparator passed in.
    vector<Item> spath(path.begin(), path.end());
    sort(spath.begin(), spath.end(), comparator);
    if (spath != path) {
      // Path needs to be sorted. Remove, then re-add it sorted.
      Remove(path, 0, count);
      Insert(spath, count);
    }
  }
  ASSERT(IsSorted());

  Log("Sort took %.3lfs\n", timer.Seconds());
}
