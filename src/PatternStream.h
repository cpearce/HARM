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
#include <memory>
#include <stdint.h>
#include "ItemSet.h"

class PatternStreamWriter;
class DataSet;

class PatternOutputStream {
public:

  // Default constructor creates a "fake" output stream, that counts
  // its output, but doesn't write it anywhere.
  PatternOutputStream() {}

  // Creates a PatternOutputStream that writes (patterns,count) to _stream.
  PatternOutputStream(std::shared_ptr<std::ostream> _stream, DataSet* index);

  PatternOutputStream& operator=(PatternOutputStream&& other);

  // Synchronously writes pattern to stream.
  void Write(const ItemSet& pattern);
  void Write(const std::vector<Item>& pattern);

  int64_t GetNumPatterns() const {
    return numPatterns;
  }

  void Close();

private:

  bool IsFakeWriter() const { return !stream && !index; }

  DataSet* index = nullptr;
  std::shared_ptr<std::ostream> stream;
  unsigned numPatterns = 0;
};

class PatternInputStream {
public:

  PatternInputStream(std::shared_ptr<std::istream> stream);

  // Returns a null itemset upon EOF.
  ItemSet Read();

  bool IsOpen() {
    return file->good();
  }

  std::vector<ItemSet> ToVector();

private:
  std::shared_ptr<std::istream> file;
};

void PatternStream_Test();

#endif
