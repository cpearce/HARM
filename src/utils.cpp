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

#include <time.h>
#include "utils.h"
#include "debug.h"
#include "Options.h"
#include <string>
#include <vector>
#include <stdarg.h>
#include <fstream>
#include "InvertedDataSetIndex.h"
#include "List.h"
#include "PatternStream.h"
#include "ItemMap.h"
#include <memory>

using namespace std;

/*
Usage:

 vector<string> tokens;
	 string str("Split,me up!Word2 Word3.");
	 Tokenize(str, tokens, ",<'> " );
	 vector <string>::iterator iter;
	 for(iter = tokens.begin();iter!=tokens.end();iter++){
	 		cout<< (*iter) << endl;
	}

*/

void Tokenize(const string& str,
              vector<string>& tokens,
              const string& delimiters) {
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);
  while (string::npos != pos || string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}


// Trims leading and trailing whitespace.
string& TrimWhiteSpace(string& str) {
  char const* delims = " \t\r\n";

  // trim leading whitespace
  string::size_type notwhite = str.find_first_not_of(delims);
  str.erase(0, notwhite);

  // trim trailing whitespace
  notwhite = str.find_last_not_of(delims);
  str.erase(notwhite + 1);

  return str;
}

string TrimWhiteSpace(const string& str) {
  string s = str;
  return TrimWhiteSpace(s);
}

uint32_t PopulationCount(uint32_t i) {
  // http://stackoverflow.com/a/109025
  i = i - ((i >> 1) & 0x55555555);
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
  return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

static FILE* gOutputLog = 0;

bool InitLog(Options& options) {
  if (gOutputLog == 0) {
    string filename = GetOutputLogFileName(options.outputFilePrefix);
    gOutputLog = fopen(filename.c_str(), "w");
    if (!gOutputLog) {
      cerr << "Fail: Can't open log file '" << filename << "'\n";
      return false;
    }
  }
  return true;
}

void CloseLog() {
  fclose(gOutputLog);
}

void Log(const char* msg, ...) {
  va_list argp;
  va_start(argp, msg);
  vfprintf(stdout, msg, argp);
  if (gOutputLog) {
    vfprintf(gOutputLog, msg, argp);
  }
  va_end(argp);
}


bool DumpItemSets(vector<ItemSet>& aResult,
                  DataSet* aIndex,
                  Options& options) {
  if (options.countItemSetsOnly) {
    return true;
  }

  string filename = GetOutputItemsetsFileName(options.outputFilePrefix);
  ofstream out(filename.c_str());
  ASSERT(out);
  if (!out) {
    cerr << "Fail: Can't open '" << filename << "' for writing\n";
    return false;
  }
  for (unsigned i = 0; i < aResult.size(); i++) {
    string s = aResult[i];
    double sup = aIndex->Support(aResult[i]);
    if (sup < options.minSup) {
      cerr << "WARNING: itemset " << s << " has support of "
           << sup << ", but minsup=" << options.minSup << endl;
    }
    out << s << "," << sup << "\n";
  }
  out.close();
  return true;
}

double Confidence(Rule& r, DataSet* aIndex) {
  ItemSet AB(r.mAntecedent, r.mConsequent);
  return (double)aIndex->Support(AB) / (double)aIndex->Support(r.mAntecedent);
}

double Lift(Rule& r, DataSet* aIndex) {
  return Confidence(r, aIndex) / (double)aIndex->Support(r.mConsequent);
}

void GenerateRulesRecursize(long& aNumRules,
                            double minConf,
                            double minLift,
                            DataSet* aIndex,
                            set<Item>::iterator itr,
                            set<Item>::iterator end,
                            ItemSet& aAntecedent,
                            ItemSet& aConsequent,
                            bool aCountRulesOnly,
                            ostream& out) {
  if (itr == end) {
    if (aConsequent.Size() == 0 || aAntecedent.Size() == 0) {
      return;
    }

    // Calculate the confidence, lift, and support of the rule.
    double confidence = 0;
    double support = 0;
    double lift = 0;

    {
      ItemSet AB(aAntecedent, aConsequent);
      support = aIndex->Support(AB);
      confidence = support / (double)aIndex->Support(aAntecedent);
      lift = confidence / (double)aIndex->Support(aConsequent);
    }

    if (confidence > minConf &&
        lift > minLift) {
      aNumRules++;
      Rule r;
      r.mAntecedent = aAntecedent;
      r.mConsequent = aConsequent;
      string s = r;
      if (!aCountRulesOnly) {
        out << s << "," << confidence << "," << lift << "," << support* aIndex->NumTransactions() << endl;
      }
    }
    return;
  }

  Item item = *itr;
  itr++;

  aAntecedent.mItems.insert(item);
  GenerateRulesRecursize(aNumRules, minConf, minLift, aIndex, itr, end, aAntecedent, aConsequent, aCountRulesOnly, out);
  aAntecedent.mItems.erase(item);

  aConsequent.mItems.insert(item);
  GenerateRulesRecursize(aNumRules, minConf, minLift, aIndex, itr, end, aAntecedent, aConsequent, aCountRulesOnly, out);
  aConsequent.mItems.erase(item);
}

void GenerateRules(PatternInputStream& input,
                   double minConf,
                   double minLift,
                   long& aNumRules,
                   DataSet* aIndex,
                   string aOutputPrefix,
                   bool countRulesOnly) {
  time_t startTime = time(0);

  aNumRules = 0;

  string filename = GetOutputRuleFileName(aOutputPrefix);
  ofstream out(filename);
  ASSERT(out.is_open());

  ItemSet candidate;
  while (!(candidate = input.Read()).IsNull()) {
    ItemSet antecedent, consequent;
    GenerateRulesRecursize(aNumRules, minConf, minLift, aIndex,
                           candidate.mItems.begin(), candidate.mItems.end(),
                           antecedent, consequent, countRulesOnly, out);
  }

  time_t timeTaken = time(0) - startTime;
  Log("Generated %d rules in %lld seconds%s\n", aNumRules, timeTaken,
      (countRulesOnly ? " (not saved to disk)" : ""));
}


void GenerateRules(const vector<ItemSet>& aItemsets,
                   double minConf,
                   double minLift,
                   long& aNumRules,
                   DataSet* aIndex,
                   string aOutputPrefix,
                   bool countRulesOnly) {
  time_t startTime = time(0);

  aNumRules = 0;

  string filename = GetOutputRuleFileName(aOutputPrefix);
  ofstream out(filename);
  ASSERT(out.is_open());

  for (unsigned i = 0; i < aItemsets.size(); i++) {
    ItemSet candidate = aItemsets[i];
    ItemSet antecedent, consequent;
    GenerateRulesRecursize(aNumRules, minConf, minLift, aIndex,
                           candidate.mItems.begin(), candidate.mItems.end(),
                           antecedent, consequent, countRulesOnly, out);
  }

  time_t timeTaken = time(0) - startTime;
  Log("Generated %d rules in %lld seconds\n", aNumRules, timeTaken);
}


void List_Test() {
  List<int> list;
  for (int i = 0; i <= 20; i++) {
    list.Append(i);
  }
  ASSERT(list.GetSize() == 21);

  {
    vector<int> L;
    List<int>::Iterator* itr = list.Begin();
    while (itr->HasNext()) {
      L.push_back(itr->Next());
    }
    for (int i = 0; i <= 20; i++) {
      ASSERT(L[i] == i);
    }
    delete itr;
  }

  List<int>::Iterator* itr = list.Begin();
  int expected = 0;
  while (itr->HasNext()) {
    List<int>::Token token = itr->GetToken();
    ASSERT(token.IsInList());
    int x = itr->Next();
    if (expected % 2 == 0) {
      list.Erase(token);
      ASSERT(!token.IsInList());
    }
    ASSERT(x == expected);
    expected++;
  }
  ASSERT(expected == 21);
  ASSERT(list.GetSize() == 10);
  ASSERT(!itr->HasNext());
  for (int i = 21; i <= 40; i++) {
    list.Append(i);
  }
  ASSERT(itr->HasNext());
  while (itr->HasNext()) {
    int x = itr->Next();
    ASSERT(x == expected);
    expected++;
  }
  ASSERT(!itr->HasNext());

  list.Prepend(-1);
  ASSERT(!itr->HasNext());
  itr = list.Begin();
  ASSERT(itr->Next() == -1);
  delete itr;
}

void ItemMap_Test() {
  std::map<int, unsigned> expected;
  ItemMap<unsigned> m;
  for (unsigned i = 1; i < 1000; ++i) {
    Item item(i);
    m.Set(item, i);
    expected[item.GetId()] = i;
  }

  ItemMap<unsigned>::Iterator itr = m.GetIterator();
  int count = 0;
  while (itr.HasNext()) {
    Item item = itr.GetKey();
    unsigned value = itr.GetValue();
    ASSERT(value == expected[item.GetId()]);
    itr.Next();
    count++;
  }
  ASSERT(count == expected.size());
}

// Converts a string in "a,b,c" form to a vector of items [a,b,c].
// Useful for testing.
std::vector<Item> ToItemVector(const std::string& _items) {
  std::vector<Item> items;
  std::vector<std::string> tokens;
  Tokenize(_items, tokens, ", ");
  items.reserve(tokens.size());
  for (const string& token : tokens) {
    items.push_back(Item(token));
  }
  return move(items);
}
