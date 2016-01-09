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

#include "DataStreamMining.h"
#include <memory>
#include "ConnectionTable.h"


class FPNode;
class VariableWindowDataSet;

class StructuralStreamDriftDetector : public StreamMiner {
public:
  // check_interval: the number of transactions between "check points", where
  // we re-evaluate the tree to see if we should throw away data and/or mine.
  StructuralStreamDriftDetector(uint32_t check_interval,
                                bool have_automatic_structure_drift_threshold,
                                bool have_structure_drift_threshold,
                                double structure_drift_threshold,
                                bool have_structure_merge_threshold,
                                double structure_merge_threshold,
                                bool have_item_drift_threshold,
                                double item_drift_threshold,
                                bool have_item_freq_merge_thresold,
                                double item_freq_merge_threshold,
                                bool ssdd_window_cmp,
                                bool dd_print_blocks,
                                bool adaptive_window,
                                bool almost_exact,
                                bool use_distribution_drift,
                                double dbdd_delta);

  void Add(Transaction& transaction) override;
  void Init(MiningContext* miner) override;

  static void Test();
  static void TestSpearman();

private:

  // Calculates tree "instability", a measure of how much change is required
  // to sort the tree with the given frequency table.
  double TreeInstability(const ItemMap<unsigned>* frequency_table) const;

  double TreeInstability(const ItemMap<unsigned>* lhs_frequency_table,
                         const ItemMap<unsigned>* rhs_frequency_table) const;

  double TreeInstability(const ItemMap<unsigned>* lhs_frequency_table,
                         const ItemMap<unsigned>* rhs_frequency_table,
                         const ConnectionTable* lhs_conn_table,
                         const ConnectionTable* rhs_conn_table) const;

  double TreeAutomaticThreshold(const ItemMap<unsigned>* lhs_frequency_table,
                                const ItemMap<unsigned>* rhs_frequency_table) const;

  double TableInstability(const ItemMap<unsigned>* frequency_table,
                          uint32_t other_block_size) const;

  // Returns the index of the last check point in check_points that is
  // "unstable", or -1 if all blocks are stable.
  int FindLastUnstableCheckPoint();

  int FindUnstableCheckPointByDistributionChange();

  // Drops all transactions in the check_points up to an including block
  // with index |block_index|.
  void PurgeBlocksUpTo(int block_index);

  int FindUnstableCheckPointByPartition();
  int FindUnstableCheckPointByProgressive();

  void MergeCheckPointWithNext(int index);

  void MaybePrintBlocks(const std::string& aMsg);

  // The number of transactions between "check points", were we re-evaluate
  // the tree to see if we should throw away data and/or mine for rules.
  const uint32_t check_interval;

  const bool have_structure_drift_threshold;
  const double structure_drift_threshold;

  // added by Minh
  const bool have_automatic_ssdd_structural_drift_threshold;
  const bool adaptive_window;
  const bool almost_exact;
  double automatic_ssdd_structural_drift_threshold;
  double automatic_ssdd_structural_merge_threshold;

  const bool have_structure_merge_threshold;
  const double structure_merge_threshold;

  const bool have_item_drift_threshold;
  const double item_drift_threshold;

  const bool have_item_freq_merge_thresold;
  const double item_freq_merge_threshold;

  const bool window_cmp;
  const bool print_blocks;

  const bool use_distribution_drift;
  const double dbdd_delta;

  std::unique_ptr<FPNode> tree;

  std::unique_ptr<VariableWindowDataSet> data_set;

  const uint32_t aw_check_interval;

  struct CheckPoint {
    // TOOD: Move ctor && etc..
    CheckPoint(TransactionId _start_tid,
               TransactionId _end_tid,
               ItemMap<unsigned>* _frequency_table
              )
      : start_tid(_start_tid),
        end_tid(_end_tid),
        frequency_table(*_frequency_table) {
    }

    CheckPoint(TransactionId _start_tid,
               TransactionId _end_tid,
               ItemMap<unsigned>* _frequency_table,
               ConnectionTable _conn_table
              )
      : start_tid(_start_tid),
        end_tid(_end_tid),
        frequency_table(*_frequency_table),
        conn_table(_conn_table) {
    }

    uint32_t Size() const {
      return end_tid - start_tid;
    }

    TransactionId start_tid;
    TransactionId end_tid;
    ItemMap<unsigned> frequency_table;
    ConnectionTable conn_table;
  };

  uint32_t SizeOfWindow(std::vector<CheckPoint>::const_iterator start,
                        std::vector<CheckPoint>::const_iterator end);

  const double purgeThreshold; // threshod in almost-exact mode

  std::vector<CheckPoint> check_points;

  MiningContext* mining_context;

  void arrangeCheckPoints(int);
  size_t capacity(CheckPoint&);
};
