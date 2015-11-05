#pragma once
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

#include <vector>
#include <map>
#include <queue>

#include "item.h"
#include "FPNode.h"
#include "ItemMap.h"
#include "ItemSet.h"

typedef ItemSet ItemList; // list of nexts (children) for each child

class Child {
  friend class ConnectionTable;

public:
  Child();
  Child(ItemList&, unsigned);

private:
  double weight; // connection weight
  unsigned count; // count filed for this item in the tree
  ItemList nextItems; // children of this child
};

typedef ItemMap<Child> ChildList; // list of children for each node
typedef ItemMap<ChildList> ConnectionList; //tree representation in a tabular format

class ConnectionTable {
public:
  ConnectionTable();
  ConnectionTable(FPNode*, ConnectionTable*);
  bool connectionExist(Item&, Item&, const ItemMap<unsigned>*) const;
  void purge(double purgeThreshold);
  void print();


private:
  void createTable(FPNode*);
  void addNewNode(FPNode*);
  void addToChildren(ChildList&, FPNode*);
  void addChildrenToQueue(FPNode*, std::queue<FPNode*>&);
  bool isConnected(Item&, Item&, const ItemMap<unsigned>*) const;
  void updateWeights(ConnectionTable&);
  void printChildren(ChildList);

  ConnectionList cl; //connection list. For every item, we keep its children. For every child we keep its children as well.
  // e.g. a: b(d, e), c(f)                      { }
  //      b: d(g), e()                           |
  //      c: f()                                 a
  //      d: g()                                / \
  //      e:                                   b   c
  //      f:                                  / \  |
  //      g:                                 d   e f
  //                                         |
  //                                         g
  FPNode* root; // root of global tree
  ChildList rootChildren; // children of root node.
  double decay; // decay value for updating connection weights
};


