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

#include <queue>
#include "FPNode.h"
#include "FPTree.h"
#include "ConnectionTable.h"
#include "ItemMap.h"
#include "Item.h"

using namespace std;

Child::Child(): weight(1), count(0) {
}

Child::Child(ItemList& _itemList, unsigned _count): weight(1), count(_count), nextItems(_itemList) {
}

ConnectionTable::ConnectionTable(): root(NULL), decay(0.99) {
}

ConnectionTable::ConnectionTable(FPNode* root, ConnectionTable* prevConnTable): decay(0.99) {
  createTable(root);
  // update weights if there is a previous table.
  if (prevConnTable) {
    updateWeights(*prevConnTable);
  }
}

void ConnectionTable::createTable(FPNode* root) {
  queue<FPNode*> q;
  FPNode* node;
  // traverse the tree and create connection table.
  q.push(root);
  while (!q.empty()) {
    node = q.front();
    // if this item does not exist in the connection table, add it as
    // a new item otherwise, just add new children if there is any.
    if (!cl.Contains(node->item)) {
      addNewNode(node);
    } else if (node->children.size() > 0) {
      // Note: this copies! Maybe ConnectionList should map to pointers
      // to child lists!!
      ChildList childList(cl.Get(node->item));
      addToChildren(childList, node);
    }

    addChildrenToQueue(node, q);
    q.pop();
  }
  //  print();
}

void ConnectionTable::addNewNode(FPNode* node) {
  ChildList children;
  ItemList nextItems;

  children.Clear();
  // go through the node's chilren and add them to an item map. For every children, add its children to an
  // nextItems.
  for (std::map<Item, FPNode*>::iterator childIter = node->children.begin(); childIter != node->children.end(); childIter++) {
    nextItems.Clear();
    for (std::map<Item, FPNode*>::iterator nextIter = childIter->second->children.begin(); nextIter != childIter->second->children.end(); nextIter++) {
      nextItems += nextIter->first;
    }
    children.Set(childIter->first, Child(nextItems, childIter->second->count));
  }

  // we cant put an item with ID 0 in ItemMap, so we do not keep the root in the connection list.
  // root does not represent any transaction, so not keeping it in the connection list should not
  // cause any problem.

  if (node->item.IsNull()) {
    root = node;
    rootChildren = children;
  } else {
    cl.Set(node->item, children);
  }
}

void ConnectionTable::addChildrenToQueue(FPNode* node, queue<FPNode*>& q) {
  for (std::map<Item, FPNode*>::iterator childIter = node->children.begin(); childIter != node->children.end(); childIter++) {
    q.push(childIter->second);
  }
}

void ConnectionTable::addToChildren(ChildList& children, FPNode* node) {
  ItemList nextItems;

  // loop through the node's children and add them new children to the list of there is any.
  for (std::map<Item, FPNode*>::iterator childIter = node->children.begin(); childIter != node->children.end(); childIter++) {
    nextItems.Clear();
    if (children.Contains(childIter->first)) {
      nextItems = children.Get(childIter->first).nextItems;
    }
    for (std::map<Item, FPNode*>::iterator nextIter = childIter->second->children.begin(); nextIter != childIter->second->children.end(); nextIter++) {
      nextItems += nextIter->first;
    }
    children.Set(childIter->first, Child(nextItems, childIter->second->count));
  }
  cl.Set(node->item, children);
}

void ConnectionTable::updateWeights(ConnectionTable& prevConnTable) {
  ConnectionList::Iterator nodeIterator = cl.GetIterator();
  // update weight field for each child in the table. Every connection in the table is assigned a weight.
  // When a new checkpoint is created, the weight of each connection is calculated based on the weight of
  // the same connection in the previous checkpoint. If the connection does not exist in the previous table,
  // then it is a new connection and gets the highest weight, which is 1. If it exists in the previous
  // table but no transaction since then contains such connection, then the weight is reduced by getting
  // multiplied by a decay value, which is 0.99. If it exists in the previous one but there is a transaction
  // which contains such connection since creating previous table, the weight resets to 1. In fact, the weight
  // shows the age of the connection in the table.
  while (nodeIterator.HasNext()) {
    Item item = nodeIterator.GetKey();
    if (prevConnTable.cl.Contains(item)) {
      ChildList prevChildren = prevConnTable.cl.Get(item);
      ChildList children = nodeIterator.GetValue();
      ChildList::Iterator childrenIterator = children.GetIterator();
      while (childrenIterator.HasNext()) {
        Child child = childrenIterator.GetValue();
        Item childItem = childrenIterator.GetKey();
        if (prevChildren.Contains(childItem)) {
          Child prevChild = prevChildren.Get(childItem);
          if (child.count == prevChild.count) {
            child.weight = prevChild.weight * decay;
          } else if (child.count > prevChild.count) {
            child.weight = 1;
          } else {
            // if the count is smaller, it means that a purge has happened. We dont know
            // if a new instance of this connection has been observed or not, so we keep
            // the weight for this the same as the previous table.
            child.weight = prevChild.weight;
          }
          children.Set(childItem, child);
        }
        childrenIterator.Next();
      }
      cl.Set(item, children);
    }
    nodeIterator.Next();
  }
}

// a -> b
bool ConnectionTable::connectionExist(Item& a, Item& b, const ItemMap<unsigned>* freqTable) const {
  bool connected = false;

  // make sure a and b exist in this connection table.
  if (!freqTable->Contains(a) || !freqTable->Contains(b)) {
    return connected;
  }

  // a->c->d...->b is an indirect connection between a and b (a->b) if the frequency of c, d, ..., b are equal.
  connected = isConnected(a, b, freqTable);
  if (!connected && (freqTable->Get(a) == freqTable->Get(b))) {
    // b->c->d...->a is an indirect connection between a and b (a->b) if the frequency of b, c, d, ..., a are equal.
    connected = isConnected(b, a, freqTable);
  }
  return connected;
}


bool ConnectionTable::isConnected(Item& a, Item& b, const ItemMap<unsigned>* freqTable) const {
  queue<Item> q;
  Item item;
  unsigned bFreq;
  static ItemSet visited;

  visited.Clear();
  bFreq = freqTable->Get(b);

  // start from a and search the connection table for direct or indirect connections between
  // a and b. a->c->d...->b is an indirect connection between a and b (a->b) if the frequency
  // of c, d, ..., b are equal.
  item = a;
  do {
    visited += item;
    ChildList* children = (ChildList*)cl.GetRef(item);
    if (children) {
      ChildList::Iterator childrenIterator = children->GetIterator();
      while (childrenIterator.HasNext()) {
        Item child = childrenIterator.GetKey();
        if (child == b) {
          return true;
        }
        // if frequency of this child is equal to b's frequency, push it to the queue
        // for further investigation, if it hasn't visited before.
        if ((freqTable->Get(child) == bFreq) && !visited.Contains(child)) {
          q.push(child);
        }
        childrenIterator.Next();
      }
    }
    if (!q.empty()) {
      item = q.front();
      q.pop();
    } else {
      break;
    }
  } while (1);

  return false;
}

void ConnectionTable::purge(double purgeThreshold) {
  unsigned n = 0;
  ConnectionList::Iterator nodeIterator = cl.GetIterator();

  // search for children with a weight less than purgeThreshold and
  // remove them.
  while (nodeIterator.HasNext()) {
    Item item = nodeIterator.GetKey();
    ChildList children = nodeIterator.GetValue();
    ChildList::Iterator childrenIterator = children.GetIterator();
    while (childrenIterator.HasNext()) {
      Item childItem = childrenIterator.GetKey();
      if (childrenIterator.GetValue().weight <= purgeThreshold) {
        children.Erase(childItem);
        n++;
      }
      childrenIterator.Next();
    }
    cl.Set(item, children);
    nodeIterator.Next();
  }

  Log("%u items were removed from connection table.\n", n);
  //  print();

}

void ConnectionTable::printChildren(ChildList children) {
  ChildList::Iterator childrenIterator = children.GetIterator();
  while (childrenIterator.HasNext()) {
    Item item = childrenIterator.GetKey();
    vector<Item> nextItems = childrenIterator.GetValue().nextItems.AsVector();
    Log("%d[%5.3f]:(", item.GetId(), childrenIterator.GetValue().weight);
    for (int i = 0; i < nextItems.size(); i++) {
      Log("%d", nextItems[i].GetId());
      if (i + 1 < nextItems.size()) {
        Log(", ");
      }
    }
    childrenIterator.Next();
    Log(")");
    if (childrenIterator.HasNext()) {
      Log(", ");
    }
  }
  Log("\n");
}

void ConnectionTable::print() {

  Log("Connection Table:\n");

  if (!root) {
    Log("Connection table is empty!\n");
    return;
  }

  Log("[root(%d)] - ", root->item.GetId());
  printChildren(rootChildren);

  ConnectionList::Iterator nodeIterator2 = cl.GetIterator();
  while (nodeIterator2.HasNext()) {
    Item item = nodeIterator2.GetKey();
    Log("[%d] - ", item.GetId());
    printChildren(nodeIterator2.GetValue());
    nodeIterator2.Next();
  }
}


