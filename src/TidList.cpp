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


#include "TidList.h"
#include "debug.h"
#include "utils.h"

using namespace std;

void TidList::Set(int aTid, bool aValue) {
  int vidx = aTid / TIDLIST_SIZE;
  int chunkIdx = TidToChunkIdx(aTid);
  unsigned chunk = GetChunk(vidx, chunkIdx);

  int bitNum = TidToChunkBitNum(aTid);
  if (aValue) {
    chunk |= (1 << bitNum);
  } else {
    chunk &= (~0 - (1 << bitNum));
  }
  //v->at(chunkIdx) = chunk;
  SetChunk(vidx, chunkIdx, chunk);

  ASSERT(Get(aTid) == aValue);
}

unsigned TidList::SetChunk(int aVecIdx, int aChunkIdx, unsigned aVal) {
  if (aVecIdx >= mChunks.size()) {
    mChunks.resize(aVecIdx + 1);
  }
  if (mChunks[aVecIdx] == 0) {
    mChunks[aVecIdx] = new vector<unsigned>();
  }
  vector<unsigned>* v = mChunks[aVecIdx];
  if (v->size() <= aChunkIdx) {
    v->resize(aChunkIdx + 1);
  }

  v->at(aChunkIdx) = aVal;

  return aVal;
}

bool TidList::Get(int aTid) const {
  int vidx = aTid / TIDLIST_SIZE;
  if (mChunks[vidx] == 0) {
    return false;
  }
  unsigned chunk = mChunks[vidx]->at(TidToChunkIdx(aTid));

  int bitNum = TidToChunkBitNum(aTid);
  return (chunk & (1 << bitNum)) != 0;
}

unsigned TidList::GetChunk(int aVecId, int aChunkId) const {
  if (aVecId >= mChunks.size() ||
      mChunks[aVecId] == 0 ||
      mChunks[aVecId]->size() <= aChunkId) {
    return 0;
  }

  return mChunks.at(aVecId)->at(aChunkId);
}

void TidList::Test() {
  TidList tl;
  tl.Set(123, true);
  ASSERT(tl.Get(123) == true);
  tl.Set(123, false);
  ASSERT(tl.Get(123) == false);

  tl.Set(31, true);
  tl.Set(32, true);
  tl.Set(33, true);
  ASSERT(tl.Get(31));
  ASSERT(tl.Get(32));
  ASSERT(tl.Get(33));

  tl.Set(31, false);
  ASSERT(!tl.Get(31));
  ASSERT(tl.Get(32));
  ASSERT(tl.Get(33));

  tl.Set(32, false);
  ASSERT(!tl.Get(31));
  ASSERT(!tl.Get(32));
  ASSERT(tl.Get(33));

  tl.Set(33, false);
  ASSERT(!tl.Get(31));
  ASSERT(!tl.Get(32));
  ASSERT(!tl.Get(33));

}

