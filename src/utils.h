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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>
#include <chrono>
#include <math.h>
#include "Options.h"
#include "ItemSet.h"
#include "PatternStream.h"

using namespace std::chrono;

class DurationTimer {
public:
  DurationTimer()
  {
    Reset();
  }
  int64_t Milliseconds() {
    return duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - mStart).count();
  }
  double Seconds() {
    return duration_cast<duration<double>>(std::chrono::high_resolution_clock::now() - mStart).count();
  }
  void Reset() {
    mStart = std::chrono::high_resolution_clock::now();
  }
private:
  std::chrono::high_resolution_clock::time_point mStart;
};

inline double round(double d) {
  return floor(d + 0.5);
}

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
void Tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters = " ");

std::string& TrimWhiteSpace(std::string& str);

std::string TrimWhiteSpace(const std::string& str);

std::vector<Item> ToItemVector(const std::string& _items);

uint32_t PopulationCount(uint32_t x);


// A Stack based class to automatically delete a ptr when it's destroyed.
template<class T>
class AutoPtr {
public:
  AutoPtr(T* aPtr) : mPtr(aPtr) {}
  ~AutoPtr() {
    if (mPtr) {
      delete mPtr;
    }
  }

  T* operator->() const {
    return mPtr;
  }

  AutoPtr<T>& operator=(T* rhs)
  // assign from a raw pointer (of the right type)
  {
    Assign(rhs);
    return *this;
  }

  AutoPtr<T>& operator=(AutoPtr<T>& rhs)
  // assign by transferring ownership from another smart pointer.
  {
    Assign(rhs.Forget());
    return *this;
  }

  T* Forget() {
    T* temp = mPtr;
    mPtr = 0;
    return temp;
  }

  operator T* () const {
    return mPtr;
  }


private:
  T* mPtr;
  void Assign(T* aPtr) {
    if (mPtr) {
      delete mPtr;
    }
    mPtr = aPtr;
  }
};

inline std::string narrow(std::wstring& wide) {
  std::string ns(wide.begin(), wide.end());
  return ns;
}

inline std::wstring wide(std::string& narrow) {
  std::wstring ws(narrow.begin(), narrow.end());
  return ws;
}

#define ARRAY_LENGTH(array_) \
  (sizeof(array_)/sizeof(array_[0]))

void Log(const char* msg, ...);

class Rule {
public:
  ItemSet mAntecedent;
  ItemSet mConsequent;
  operator std::string() const {
    std::string a = mAntecedent;
    std::string c = mConsequent;
    return a + " -> " + c;
  }
};

bool DumpItemSets(std::vector<ItemSet>& aResult, DataSet* aIndex, Options& options);

void GenerateRules(const std::vector<ItemSet>& aItemsets,
                   double minConf,
                   double minLift,
                   long& aNumRules,
                   DataSet* aIndex,
                   std::string aOutputPrefix,
                   bool countRulesOnly);

void GenerateRules(PatternInputStream& input,
                   double minConf,
                   double minLift,
                   long& aNumRules,
                   DataSet* aIndex,
                   std::string aOutputPrefix,
                   bool countRulesOnly);

#endif
