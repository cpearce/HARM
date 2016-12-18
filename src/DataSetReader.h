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
#include <fstream>
#include <unordered_set>

#include <time.h>
#include <stdlib.h>
#include <memory>

#include "Item.h"
#include "debug.h"
#include "utils.h"

// Class that encapsulates reading a CSV file and parsing it as transactions.
class DataSetReader {
public:

  DataSetReader(std::unique_ptr<std::istream> aInput)
    : mInput(move(aInput))
  {
  }

  bool IsGood() {
    return mInput->good();
  }

  bool Rewind() {
    mLineNumber = 0;
    mInput->clear(); // Clear EOF flag if necessary.
    mInput->seekg(std::istream::beg);
    return IsGood();
  }

  // Returns the next transaction in the file. Note duplicate items in
  // the transaction are removed, items only appear once in a transaction,
  // even if they're appear multiple times in the actual file.
  // Returns true if not at end of file.
  bool GetNext(std::vector<Item>& aTransaction) {
    std::string line;
    aTransaction.clear();
    aTransaction.reserve(20);
    if (!getline(*mInput, line)) {
      return false;
    }
    ++mLineNumber;
    std::vector<std::string> tokens;
    Tokenize(line, tokens, ",");
    if (tokens.size() == 0) {
      std::cerr << "Null transaction on line '" << mLineNumber << "' failing!\n";
      exit(-1);
    }

    std::vector<std::string>::iterator itr;
    std::unordered_set<std::string> items;
    for (itr = tokens.begin(); itr != tokens.end(); itr++) {
      std::string itemName = TrimWhiteSpace(*itr);
      if (itemName.empty()) {
        std::cerr << "Empty or all whitespace transaction on line '" << mLineNumber << "' failing!\n";
        exit(-1);
      }
      if (itemName.find(" ") != std::string::npos) {
        std::cerr << "ERROR: Item name '" << itemName.c_str() << " on line "
                  << mLineNumber << " contains spaces, this won't work with the HARM's "
                  " puny itemset parser. Remove all spaces from your dataset! Aborting!" << std::endl;
        exit(-1);
      }
      if (items.find(itemName) == items.end()) {
        // Item does not already appear in this transaction, add it.
        // This is how we prevent duplicate items appearing in the transaction.
        // We rely on assuming that transactions don't include the same item twice.
        aTransaction.push_back(Item(itemName));
        items.insert(itemName);
      }
    }
    return true;
  }
private:
  std::unique_ptr<std::istream> mInput;
  uint64_t mLineNumber = 0;
};
