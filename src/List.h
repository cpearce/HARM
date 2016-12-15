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

#ifndef __LIST_H__
#define __LIST_H__

#include <set>
#include <assert.h>

// Double linked list which an handle removal during iteration, and provides
// tokens to enable O(1) deletion.
template<class T>
class List {
private:

  class Node {
    friend class List;
    Node* next;
    Node* prev;
    T val;
    Node(T v) : next(0), prev(0), val(v) {
    }
    Node(T v, Node* n, Node* p)
      : next(n), prev(p), val(v) {
      if (next) {
        next->prev = this;
      }
      if (prev) {
        prev->next = this;
      }
    }
    ~Node() {

    }
  };

  Node* head;
  Node* tail;
  unsigned size;

public:

  class Token {
    friend class List;
  public:
    bool IsInList() const {
      return node != 0;
    }
    Token() : node(0) {}
  private:
    Token(Node* aNode) : node(aNode) {}

    Node* node;
  };

  List()
    : head(0)
    , tail(0)
    , size(0)
  {
  }

  ~List() {
    Node* n = head;
    while (n) {
      Node* t = n;
      n = n->next;
      delete t;
    }
  }

  unsigned GetSize() const {
    return size;
  }

  Token Append(T val) {
    if (!tail) {
      assert(!head);
      tail = head = new Node(val);
    } else {
      Node* n = new Node(val, 0, tail);
      tail = n;
    }
    auto itr = iterators.begin();
    while (itr != iterators.end()) {
      Iterator* i = *itr;
      i->NotifyAppended(tail);
      itr++;
    }
    size++;
    return Token(tail);
  }

  Token Prepend(T val) {
    if (!head) {
      assert(!tail);
      head = tail = new Node(val);
    } else {
      head = new Node(val, head, 0);
    }
    size++;
    return Token(head);
  }

  // Remove the value for token from the list. Does not delete the value
  // associated with token, caller is responsible for that.
  void Erase(Token& token) {
    if (!token.IsInList()) {
      return;
    }
    Node* node = token.node;

    // Notify all iterators that the node has been removed.
    auto itr = iterators.begin();
    while (itr != iterators.end()) {
      Iterator* i = *itr;
      i->NotifyDeleted(node);
      itr++;
    }

    Node* next = node->next;
    Node* prev = node->prev;
    if (prev) {
      prev->next = next;
    } else {
      // Deleting head.
      head = next;
    }
    if (next) {
      next->prev = prev;
    } else {
      // Deleting tail.
      tail = prev;
    }
    delete node;
    token.node = 0;
    size--;
  }

  class Iterator {
    friend class List;
  public:
    Token GetToken() {
      return Token(node);
    }

    T Next() {
      assert(HasNext());
      T val = node->val;
      node = node->next;
      return val;
    }

    bool HasNext() {
      return node != 0;
    }

    ~Iterator() {
      list->iterators.erase(this);
    }
  private:

    void NotifyDeleted(Node* n) {
      if (n == node) {
        node = node->next;
      }
    }

    void NotifyAppended(Node* n) {
      if (node == 0) {
        node = n;
      }
    }

    Iterator(Node* _n, List* l) : node(_n), list(l) {}
    Node* node;
    List* list;
  };

  // Caller must delete iterator when finished with it.
  // Forward iterator only.
  Iterator* Begin() {
    Iterator* itr = new Iterator(head, this);
    iterators.insert(itr);
    return itr;
  }

private:
  std::set<Iterator*> iterators;
};

#endif
