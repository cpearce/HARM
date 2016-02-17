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
        HeaderTable().Set(item, next);
        next->prev = nullptr;
      } else {
        // No next, just erase the entry for this item.
        HeaderTable().Erase(item);
      }
    } else {
      if (next) {
        next->prev = prev;
      }
      prev->next = next;
    }
    if (IsLeaf()) {
      ASSERT(leafToken.IsInList());
      Leaves().Erase(leafToken);
      ASSERT(!leafToken.IsInList());
    }
    if (count) {
      Decrement(count);
    }
  }
  // Delete our child trees.
  map<Item, FPNode*>::iterator itr = children.begin();
  while (itr != children.end()) {
    delete itr->second;
    itr++;
  }
  children.clear();
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
                                  const ItemMap<unsigned>& freq) {
  for (unsigned i = 1; i < v.size(); i++) {
    Item prev = v[i - 1];
    Item curr = v[i];

    unsigned curCount = freq.Get(curr, 0);
    unsigned prevCount = freq.Get(prev, 0);
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
  if (!FrequencyTableAtLastSort().IsEmpty()) {
    ItemMapCmp<unsigned> cmp(FrequencyTableAtLastSort());
    sort(transaction.begin(), transaction.end(), cmp);
    ASSERT(VerifySortedByFreqAppearance(transaction, FrequencyTableAtLastSort()));
  } else {
    AppearanceCmp cmp;
    sort(transaction.begin(), transaction.end(), cmp);
    ASSERT(VerifySortedByAppearance(transaction));
  }
}

FPNode* FPNode::GetChild(Item aItem) const
{
  map<Item, FPNode*>::const_iterator itr = children.find(aItem);
  return (itr != children.end()) ? itr->second : nullptr;
}

FPNode* FPNode::GetOrCreateChild(Item aItem)
{
  FPNode* node = GetChild(aItem);
  if (!node) {
    // Item is not in child list, create a new node for it.
    node = new FPNode(mTree, aItem, this, 0, depth + 1);
    ASSERT(node->leafToken.IsInList());

    if (!IsRoot() && IsLeaf()) {
      // We're about to add a child node, so we'll stop being a leaf.
      // Remove us from the leaves list.
      ASSERT(leafToken.IsInList());
      Leaves().Erase(leafToken);
      ASSERT(!leafToken.IsInList());
    }
    children[aItem] = node;
    ASSERT(!IsLeaf());

    // Add new item to the headerTable.
    AddToHeaderTable(node);
  }
  return node;
}

void FPNode::Insert(std::vector<Item>::const_iterator aBegin,
                    std::vector<Item>::const_iterator aEnd,
                    unsigned aCount) {
  FPNode* parent = this;
  for (aBegin; aBegin != aEnd; aBegin++) {
    Item item = *aBegin;
    FPNode* node = parent->GetOrCreateChild(item);
    node->count += aCount;
    FrequencyTable().Increment(item, aCount);
    parent = node;
  }
}

void FPNode::Remove(const std::vector<Item>& path)
{
  Remove(path.begin(), path.end(), 1);
}

void FPNode::Decrement(uint32_t aCount)
{
  ASSERT(count >= aCount);
  count -= aCount;
  ASSERT(FrequencyTable().Get(item) >= aCount);
  ASSERT(FrequencyTable().Contains(item));
  FrequencyTable().Decrement(item, aCount); 
}

void FPNode::Remove(std::vector<Item>::const_iterator aPathBegin,
                    std::vector<Item>::const_iterator aPathEnd,
                    unsigned aCount)
{
  // Traverse path, and decrement frequency. If we lower a node to count 0,
  // we can stop and delete the remainder of the subtree/path.
  FPNode* parent = this;
  for (auto pathItr = aPathBegin; pathItr != aPathEnd; pathItr++) {
    Item item = *pathItr;
    auto itr = parent->children.find(item);
    ASSERT(itr != parent->children.end());
    FPNode* node = itr->second;
    ASSERT(node);

    ASSERT(node->count >= aCount);
    node->Decrement(aCount);

    if (node->count == 0) {
      // End of the line! No more children can have non-zero path.
      ASSERT(node->parent == parent);
      parent->children.erase(itr);
      if (parent->children.size() == 0 && !parent->IsRoot()) {
        parent->leafToken = Leaves().Append(parent);
      }
      delete node;
      break;
    }
    parent = node;
  }
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
  if (HeaderTable().Contains(node->item)) {
    FPNode* list = HeaderTable().Get(node->item);
    node->next = list;
    ASSERT(!list->prev);
    list->prev = node;
  } else {
    ASSERT(!node->prev);
    ASSERT(!node->next);
  }
  HeaderTable().Set(node->item, node);
}


bool FPNode::DoIsSorted() const {
  ASSERT(!IsRoot());
  map<Item, FPNode*>::const_iterator itr = children.begin();
  while (itr != children.end()) {
    Item childItem = itr->first;
    unsigned f = FrequencyTable().Get(item, 0);
    unsigned cf = FrequencyTable().Get(childItem, 0);
    if (cf == f && item.GetId() > childItem.GetId()) {
      ASSERT(false);
      return false;
    }
    if (cf > f) {
      ASSERT(false);
      return false;
    }
    FPNode* childNode = itr->second;
    if (!childNode->DoIsSorted()) {
      ASSERT(false);
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
        ASSERT(false);
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
  ItemMap<unsigned>::Iterator itr = FrequencyTable().GetIterator();
  while (itr.HasNext()) {
    Item item = itr.GetKey();
    unsigned count = itr.GetValue();
    printf("%s %u\n", ((string)item).c_str(), count);
    itr.Next();
  }
}

TreePathIterator::TreePathIterator(FPTree* aTree)
  : root(aTree->GetRoot())
  , itr(aTree->Leaves().Begin())
{
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

  FrequencyTableAtLastSort() = FrequencyTable();
  ItemMapCmp<unsigned> cmp(FrequencyTable());
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

  TreePathIterator itr(mTree);
  vector<Item> path;
  unsigned count;
  while (itr.GetNext(path, count)) {
    // Sort path as per comparator passed in.
    vector<Item> spath(path.begin(), path.end());
    sort(spath.begin(), spath.end(), comparator);
    if (spath != path) {
      // Path after sort is different. We need to replace the now
      // unsorted path in the tree with a sorted path.
      Replace(path, spath, count);
    }
  }
  ASSERT(IsSorted());

  Log("Sort took %.3lfs\n", timer.Seconds());
}

void FPNode::Replace(const vector<Item>& from,
                     const vector<Item>& to,
                     uint32_t count)
{
  ASSERT(IsRoot());

  // Walk down the two paths to find where they differ, then we only need to
  // remove and reinsert the difference in path.
  FPNode* node = this;
  ASSERT(from.size() == to.size());
  size_t index = 0;
  while (index < from.size()) {
    if (from[index] != to[index]) {
      break;
    }
    node = node->GetChild(from[index]);
    ASSERT(node);
    index++;
  }

  node->Remove(from.begin() + index, from.end(), count);
  node->Insert(to.begin() + index, to.end(), count);
}


ItemMap<FPNode*>&
FPNode::HeaderTable() const {
  ASSERT(mTree);
  return mTree->HeaderTable();
}

ItemMap<unsigned>&
FPNode::FrequencyTable() const {
  ASSERT(mTree);
  return mTree->FrequencyTable();
}

ItemMap<unsigned>&
FPNode::FrequencyTableAtLastSort() const {
  ASSERT(mTree);
  return mTree->FrequencyTableAtLastSort();
}

List<FPNode*>&
FPNode::Leaves() const {
  ASSERT(mTree);
  return mTree->Leaves();
}
