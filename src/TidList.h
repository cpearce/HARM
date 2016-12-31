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

#ifndef __TIDLIST_H__
#define __TIDLIST_H__

#include <vector>

// Stores a compressed TID list. Stores conceptually as a bitset, however
// we only actually store the "chunks" of the bitset which have non-zero
// value. Chunks are stored in a map, mapped by chunk id to chunk. Chunk
// id is (tid / 8*sizeof(unsigned)).
class TidList {
public:

#define TIDLIST_SIZE 512

  // Returns the id of the chunk in mChunks.
  int TidToChunkIdx(int aTid) const {
    return (aTid % TIDLIST_SIZE) / (8 * sizeof(unsigned));
  }
  // Returns the offset of the tid'd bit in the chunk, [0..31].
  int TidToChunkBitNum(int aTid) const {
    return aTid % (8 * sizeof(unsigned));
  }

  unsigned GetChunk(size_t aVecId, size_t aChunkId) const;
  unsigned SetChunk(size_t aVecIdx, size_t aChunkIdx, unsigned aVal);

  void Set(int aTid, bool aValue);
  bool Get(int aTid) const;

  // Maps chunk id to the bitset for that chunk.
  // Indexed by aTid/32
  std::vector<std::vector<unsigned>*> mChunks;

protected:
  unsigned mCount;
};

#endif
