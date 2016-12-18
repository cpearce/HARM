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

#include "PatternStream.h"
#include "InvertedDataSetIndex.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>
#include <string>

using namespace std;

PatternOutputStream::PatternOutputStream(shared_ptr<ostream> _stream,
                                         DataSet* _index)
  : index(_index)
  , numPatterns(0)
  , stream(move(_stream))
{
}

PatternOutputStream&
PatternOutputStream::operator=(PatternOutputStream&& other)
{
  index = other.index;
  stream = move(other.stream);
  numPatterns = other.numPatterns;
  return *this;
}

void PatternOutputStream::Write(const vector<Item>& pattern) {
  if (pattern.size() == 0) {
    return;
  }

  if (IsFakeWriter()) {
    numPatterns++;
    return;
  }

  ItemSet itemset;
  for (int i = pattern.size() - 1; i >= 0; --i) {
    itemset.Add(pattern[i]);
  }
  Write(itemset);
}

void PatternOutputStream::Write(const ItemSet& itemset) {
  if (itemset.IsNull()) {
    return;
  }

  numPatterns++;
  if (IsFakeWriter()) {
    return;
  }

  string s = itemset;
  double sup = (index) ? index->Support(itemset) : 0;
  (*stream) << s << "," << sup << "\n";
}

void PatternOutputStream::Close() {
  ASSERT(IsFakeWriter() || !stream || stream->good());
  if (stream) {
    stream->flush();
  }
}

PatternInputStream::PatternInputStream(std::shared_ptr<std::istream> stream)
  : file(move(stream))
{
}

vector<ItemSet> PatternInputStream::ToVector() {
  vector<ItemSet> v;
  ASSERT(file->good());
  ItemSet i;
  while (!(i = Read()).IsNull()) {
    v.push_back(i);
  }
  return v;
}

ItemSet PatternInputStream::Read() {
  if (!file->good() || file->eof()) {
    return ItemSet();
  }
  string line;
  vector<string> tokens;
  if (!getline(*file, line)) {
    return ItemSet();
  }
  size_t end = line.rfind(",");
  if (end == string::npos) {
    return ItemSet();
  }

  line.resize(end);
  ASSERT(line.rfind(",") == string::npos);
  Tokenize(line, tokens, " ");

  ItemSet itemset;
  for (unsigned i = 0; i < tokens.size(); ++i) {
    Item item(tokens[i]);
    itemset.Add(item);
  }
  return itemset;
}
