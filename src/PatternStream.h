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

#ifndef __PATTERN_STREAM_H__
#define __PATTERN_STREAM_H__

#include <fstream>
#include <vector>
#include <stdint.h>
#include "ItemSet.h"

class PatternStreamWriter;
class DataSet;

class PatternOutputStream {
public:
  PatternOutputStream(const char* filename, DataSet* index);

  // Syncrhonously writes pattern to disk.
  void Write(const ItemSet& pattern);
  void Write(const std::vector<Item>& pattern);

  int64_t GetNumPatterns() const {
    return numPatterns;
  }

  void Close();

private:
  DataSet* index;
  std::ofstream stream;
  unsigned numPatterns;
  bool isFakeWriter;
};

class PatternInputStream {
public:

  // Returns null on file not found or error.
  bool Open(const char* filename);

  // Returns a null itemset upon EOF.
  ItemSet Read();

  bool IsOpen() {
    return file.is_open();
  }

  std::vector<ItemSet> ToVector();

private:
  std::ifstream file;
};

void PatternStream_Test();

#endif
