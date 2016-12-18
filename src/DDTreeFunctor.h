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

#include "FPTree.h"
#include "FPNode.h"
#include "utils.h"
#include "ExtrapTreeFunctor.h"
#include "CPTreeFunctor.h"

#include <vector>
#include <string>
#include <fstream>
#include <stdint.h>

//----------------------------------------------
template <class T> unsigned int edit_distance(const T& s1, const T& s2) {
  const size_t len1 = s1.size(), len2 = s2.size();
  std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

  d[0][0] = 0;
  for (unsigned int i = 1; i <= len1; ++i) {
    d[i][0] = i;
  }
  for (unsigned int i = 1; i <= len2; ++i) {
    d[0][i] = i;
  }

  for (unsigned int i = 1; i <= len1; ++i)
    for (unsigned int j = 1; j <= len2; ++j)

      d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1),
                         d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
  /*cout<<"\n";
  for(unsigned int j = 0; j <= len2; ++j){
  	for(unsigned int i = 0; i <= len1; ++i){
                    cout<<setw(3)<<d[i][j]<<" ";
  	}
  	cout<<endl;
  }*/
  return d[len1][len2];
}

/*
private int getEditDistance(String s, String t) {
    int[][] matrix = new int[s.length() + 1][t.length() + 1];

    for(int i = 0; i <= s.length(); ++i) {
        matrix[i][0] = i;
    }
    for(int i = 0; i <= t.length(); ++i) {
        matrix[0][i] = i;
    }

    for(int i = 1; i <= s.length(); ++i) {
        for(int j = 1; j <= t.length(); ++j) {
            matrix[i][j] =
              Math.min(matrix[i-1][j] + 1,
                        Math.min(matrix[i][j-1] + 1,
                                matrix[i-1][j-1] + ((s.charAt(i-1) == t.charAt(j-1)) ? 0 : 1)));
        }
    }
    //printMatrix(matrix);
    return matrix[s.length()][t.length()];
}
*/
template<class T>
unsigned int edit_distance_levenshtein(const T& s1, const T& s2) {
  const size_t len1 = s1.size(), len2 = s2.size();

  std::vector<unsigned int> col(len2 + 1), prevCol(len2 + 1);

  for (unsigned int i = 0; i < prevCol.size(); i++) {
    prevCol[i] = i;
  }
  for (unsigned int i = 0; i < len1; i++) {
    col[0] = i + 1;
    for (unsigned int j = 0; j < len2; j++)
      col[j + 1] = std::min(std::min(1 + col[j], 1 + prevCol[1 + j]),
                       prevCol[j] + (s1[i] == s2[j] ? 0 : 1));
    col.swap(prevCol);
  }
  return prevCol[len2];
}

#define MS_YES 1
#define MS_NO 0
class CmpExtrapCP {
public:
  double getSimilarity(std::vector<CmpNode>& ex, std::vector<CmpNode>& cp, int th) {
    int ex_count = ex.size();
    int cp_count = cp.size();
    //matching score
    int ms = 0;
    bool found;
    CmpNode ex_cur;
    for (int i = 0; i < ex_count; i++) {
      ex_cur = ex[i];
      found = false;
      for (int j = 0; j < cp_count; j++) {
        if (ex_cur.nId == cp[j].nId) {
          if ((cp[j].nDepth - th) <= ex_cur.nDepth && (cp[j].nDepth + th) >= ex_cur.nDepth) {
            ms += MS_YES;
            found = true;
          }
        }
        if (found) {
          break;
        }
      }
    }
    return ((ms * 100.0) / ex_count);
  }
  //This function can be used to find structure difference
  double getSimilarityWV(std::vector<CmpNode>& ex, std::vector<CmpNode>& cp, int th) {
    int ex_count = ex.size();
    int cp_count = cp.size();
    int ms = 0;
    _m.resize(ex_count);
    CmpNode ex_cur;
    for (int i = 0; i < ex_count; i++) {
      ex_cur = ex[i];
      _m[i] = MS_NO;
      for (int j = 0; j < cp_count; j++) {
        if (ex_cur.nId == cp[j].nId) {
          if ((cp[j].nDepth - th) <= ex_cur.nDepth && (cp[j].nDepth + th) >= ex_cur.nDepth) {
            _m[i] = MS_YES;
            ms += MS_YES;
          }
        }
        if (_m[i] == MS_YES) {
          break;
        }
      }
    }

    return ((ms * 100.0) / ex_count);
  }
protected:
  //Can be useful to find structure difference
  std::vector<int> _m;
};

class DDTreeFunctor : public FPTreeFunctor {
public:

  void OnStartLoad(std::unique_ptr<DataSetReader>& aReader) {

  }

  void OnLoad(const std::vector<Item>& txn) {
    ExtrapFunctor->OnLoad(txn);
    CpFunctor->OnLoad(txn);
    count++;
    if (count == windowSize) {
      interval_count++;
      Log("Similarity count between Extrap and CP \n Stop depth: %d \n", stopDepth);
      Log(" Cmp depth threshold: %d \n", cmpDepthTh);
      if (interval_count == 1) {
        if (stopDepth) {
          ExtrapFunctor->mTree->ToVector(stopDepth, &extrap_vd);
        } else {
          ExtrapFunctor->mTree->ToVector(&extrap_vd);
        }
        Log(" Number of nodes (extrap): (%d)\n", extrap_vd.size());
        Log(" Similarity measure: ignore first interval\n");
      } else {
        cp_vd.clear();
        DurationTimer timer;
        if (stopDepth) {
          CpFunctor->mTree->ToVector(stopDepth, &cp_vd);
        } else {
          CpFunctor->mTree->ToVector(&cp_vd);
        }
        Log(" Number of nodes (extrap, cp): (%d, %d)\n", extrap_vd.size(), cp_vd.size());
        cur_sm = cmp.getSimilarity(extrap_vd, cp_vd, cmpDepthTh);
        Log(" Current matching : %lf % \n", cur_sm);
        extrap_vd.clear();
        if (stopDepth) {
          ExtrapFunctor->mTree->ToVector(stopDepth, &extrap_vd);
        } else {
          ExtrapFunctor->mTree->ToVector(&extrap_vd);
        }
        Log(" Similarity measure with Extrap & CP trees traversal took %.5lfs\n", timer.Seconds());
        if (fpResultsFlg) {
          *fpResults << "\t" << cur_sm << "\t" << timer.Seconds();
        }
      }
      if (interval_count > 2) {
        Log(" Previous matching : %lf % \n", pre_sm);
        double sm_diff = cur_sm - pre_sm;
        //detect drift
        if (std::abs(sm_diff) >= driftThreshold) {
          int ddPos =  count_dft + count;
          Log(" ****** Drift detected at poistion: %d ****\n", ddPos);
          //1 mean drift is detected
          interval_count = 1;
          Log(" Init interval to :%d --", this->mBlockSize);
          Log(" (%lf, %lf) \n", sm_diff, driftThreshold);
          initInterval(this->mBlockSize);
          if (fpResultsFlg) {
            *fpResults << "\t" << 1 << "\t" << ddPos << "\t" << inputfile << "\t"
                       << driftThreshold << "\t" << this->stopDepth << "\t" << this->cmpDepthTh;
          }
        } else { // change block size
          ChangeInterval(sm_diff);
        }
      }
      pre_sm = cur_sm;
      Log("***********************************************\n");
      if (fpResultsFlg) {
        *fpResults << std::endl;
      }
      count_dft += count;
      count = 0;
    }

  }

  void OnUnload(const std::vector<Item>& txn) {
    ExtrapFunctor->OnUnload(txn);
    CpFunctor->OnUnload(txn);
  }

  void OnEndLoad() override {
    Log("Overall time DDtree %.3lfs\n", overal_timer.Seconds());
    if (mOptions.ddResultsFile.is_open()) {
      mOptions.ddResultsFile << "\t\t\t\t\t" << mOptions.inputFileName << "\t\t\t\t"
                             << overal_timer.Seconds() << "\t Overall time" << std::endl;
    }
    FPTreeFunctor::OnEndLoad();
  }

  void ChangeInterval(double sm_diff) {
    Log(" Similarity difference: %lf \n", sm_diff);
    //increment interval with +ve sm_diff
    //Call the function which Yun Sing mentioned
    double diff = (sm_diff * this->windowSize) / 100.0;
    this->windowSize += int(diff);
    this->ExtrapFunctor->windowSize  += int(diff);
    this->CpFunctor->interval += int(diff);
    Log(" Interval changed by :%d\n", int(diff));
  }

  //After drift init interval
  void initInterval(int val) {
    this->windowSize = val;
    this->ExtrapFunctor->windowSize  = val;
    this->CpFunctor->interval = val;
  }

  DDTreeFunctor(FPTree* aCpTree,
                bool useKernelRegression,
                int aDataPoints,
                DataSet* aIndex,
                unsigned interval,    /* Same as Extrap aThreshold */
                int aBlockSize,
                bool aIsStreaming,
                const std::vector<unsigned>& aTxnNums,
                Options& aOptions)
    : FPTreeFunctor(NULL, aTxnNums, aBlockSize, aIsStreaming, aIndex, aOptions),
      count(0),
      windowSize(aBlockSize),
      interval_count(0),
      count_dft(0),
      extrap_tree(new FPTree()) {
    ExtrapFunctor = new ExtrapTreeFunctor(extrap_tree, interval, aBlockSize,
                                          aIsStreaming, aTxnNums, aIndex,
                                          useKernelRegression, aDataPoints, aOptions);
    CpFunctor = new CpTreeFunctor(aCpTree, interval, aBlockSize,
                                  aIsStreaming, aTxnNums, aIndex, aOptions);
    stopDepth = aOptions.stopDepth;
    cmpDepthTh = aOptions.cmpDepthThreshold;
    driftThreshold = aOptions.driftThreshold;
    pre_sm = cur_sm = 0.0;
    fpResults = &aOptions.ddResultsFile;
    fpResultsFlg = fpResults->is_open();
    inputfile = aOptions.inputFileName;

    Log("\nDDTreeMiner");
    Log("\nLoading dataset into cpTree and extrapTree...\n");
    if (mOptions.ddResultsFile.is_open()) {
      mOptions.ddResultsFile << mOptions.inputFileName << std::endl;
    }
    overal_timer.Reset();
  }

  ExtrapTreeFunctor* ExtrapFunctor;
  CpTreeFunctor* CpFunctor;
  std::vector<CmpNode> extrap_vd;
  std::vector<CmpNode> cp_vd;
  int count;
  int windowSize;
  //for drift detection
  int interval_count;
  int stopDepth;
  int count_dft;
  //Depth threshold for comparison
  int cmpDepthTh;
  CmpExtrapCP cmp;
  //for adaptive interval
  double pre_sm;
  double cur_sm;
  double driftThreshold;
  // for final drift results
  bool fpResultsFlg;
  std::ofstream* fpResults;
  std::string inputfile;
  DurationTimer overal_timer;

  // The functor owns the sub-tree, enforcing that it's destroyed on shutdown.
  AutoPtr<FPTree> extrap_tree;
};