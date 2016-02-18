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

#include "StructuralStreamDriftDetector.h"
#include "FPNode.h"
#include "FPTree.h"
#include "InvertedDataSetIndex.h"
#include "VariableWindowDataSet.h"
#include <vector>
#include <algorithm>
#include "ConnectionTable.h"
#include <cmath>

using namespace std;

// Adapted from code at:
// http://www.lemoda.net/c/levenshtein/l.c
template<class T>
static int edit_distance(const T& v1, const T& v2) {
  const size_t len1 = v1.size();
  const size_t len2 = v2.size();
  vector<vector<int>> matrix(len1 + 1, vector<int>(len2 + 1, 0));
  //int matrix[len1 + 1][len2 + 1];
  for (int i = 0; i <= len1; i++) {
    matrix[i][0] = i;
  }
  for (int i = 0; i <= len2; i++) {
    matrix[0][i] = i;
  }
  for (int i = 1; i <= len1; i++) {
    auto c1 = v1[i - 1];
    for (int j = 1; j <= len2; j++) {
      auto c2 = v2[j - 1];
      if (c1 == c2) {
        matrix[i][j] = matrix[i - 1][j - 1];
      } else {
        int deletion = matrix[i - 1][j] + 1;
        int insert = matrix[i][j - 1] + 1;
        int substitute = matrix[i - 1][j - 1] + 1;
        int minimum = deletion;
        if (insert < minimum) {
          minimum = insert;
        }
        if (substitute < minimum) {
          minimum = substitute;
        }
        matrix[i][j] = minimum;
      }
    }
  }
  return matrix[len1][len2];
}


StructuralStreamDriftDetector::StructuralStreamDriftDetector(
  uint32_t _check_interval,
  bool _have_automatic_ssdd_structural_drift_threshold,
  bool _have_structure_drift_threshold,
  double _structure_drift_threshold,
  bool _have_structure_merge_threshold,
  double _structure_merge_threshold,
  bool _have_item_drift_threshold,
  double _item_drift_threshold,
  bool _have_item_freq_merge_thresold,
  double _item_freq_merge_threshold,
  bool _window_cmp,
  bool _print_blocks,
  bool _adaptive_window,
  bool _almost_exact,
  bool _use_distribution_drift,
  double _dbdd_delta)
  : check_interval(_check_interval),
    have_structure_drift_threshold(_have_structure_drift_threshold),
    structure_drift_threshold(_structure_drift_threshold),
    have_automatic_ssdd_structural_drift_threshold(_have_automatic_ssdd_structural_drift_threshold),
    adaptive_window(_adaptive_window),
    almost_exact(_almost_exact),
    have_structure_merge_threshold(_have_structure_merge_threshold),
    structure_merge_threshold(_structure_merge_threshold),
    have_item_drift_threshold(_have_item_drift_threshold),
    item_drift_threshold(_item_drift_threshold),
    have_item_freq_merge_thresold(_have_item_freq_merge_thresold),
    item_freq_merge_threshold(_item_freq_merge_threshold),
    window_cmp(_window_cmp),
    print_blocks(_print_blocks),
    tree(new FPTree()),
    data_set(new VariableWindowDataSet()),
    aw_check_interval(1032), //1032
    purgeThreshold(0.5),
    use_distribution_drift(_use_distribution_drift),
    dbdd_delta(_dbdd_delta)
{
}

void StructuralStreamDriftDetector::Init(MiningContext* miner) {
  mining_context = miner;
}

double StructuralStreamDriftDetector::TreeInstability(
  const ItemMap<unsigned>* frequency_table) const {
  double instability = 0.0;
  uint32_t path_count = 0;

  ItemMapCmp<unsigned> cmp(*frequency_table);

  TreePathIterator itr(tree.get());
  vector<Item> path;
  unsigned ignored;
  while (itr.GetNext(path, ignored)) {
    ++path_count;
    // Sort path based on current frequency table.
    vector<Item> spath(path.begin(), path.end());
    sort(spath.begin(), spath.end(), cmp);
    int dist = edit_distance(path, spath);
    if (dist) {
      // Path is not sorted. Record edit distance between sorted and unsorted.
      instability += double(dist) / path.size();
    }
  }
  return instability / path_count;
}

double StructuralStreamDriftDetector::TreeInstability(
  const ItemMap<unsigned>* lhs_frequency_table,
  const ItemMap<unsigned>* rhs_frequency_table) const {
  double instability = 0.0;
  uint32_t path_count = 0;

  ItemMapCmp<unsigned> cmp_lhs(*lhs_frequency_table);
  ItemMapCmp<unsigned> cmp_rhs(*rhs_frequency_table);

  TreePathIterator itr(tree.get());
  vector<Item> path;
  unsigned ignored;
  while (itr.GetNext(path, ignored)) {
    ++path_count;

    // Sort the path in the current tree based on the LHS's frequency table.
    vector<Item> lhs_spath(path.begin(), path.end());
    sort(lhs_spath.begin(), lhs_spath.end(), cmp_lhs);

    // Sort the path in the current tree based on the RHS's frequency table.
    vector<Item> rhs_spath(path.begin(), path.end());
    sort(rhs_spath.begin(), rhs_spath.end(), cmp_rhs);

    int dist = edit_distance(lhs_spath, rhs_spath);
    if (dist) {
      // Path is not sorted. Record edit distance between sorted and unsorted.
      instability += pow(double(dist) / path.size(),2);
    }
  }
  return instability / path_count;
}

double StructuralStreamDriftDetector::TreeInstability(
  const ItemMap<unsigned>* lhs_frequency_table,
  const ItemMap<unsigned>* rhs_frequency_table,
  const ConnectionTable* lhs_conn_table,
  const ConnectionTable* rhs_conn_table) const {
  double instability = 0.0;
  uint32_t path_count = 0;

  ItemMapCmp<unsigned> cmp_lhs(*lhs_frequency_table);
  ItemMapCmp<unsigned> cmp_rhs(*rhs_frequency_table);

  TreePathIterator itr(tree.get());
  vector<Item> path;
  unsigned ignored;

  // find items with frequency 0 in rhs_frequency_table
  vector<Item> zero_freqs;
  ItemMap<unsigned>::Iterator freq_iterator = rhs_frequency_table->GetIterator();
  while (freq_iterator.HasNext()) {
    Item item = freq_iterator.GetKey();
    if (freq_iterator.GetValue() == 0) {
      zero_freqs.push_back(item);
    }
    freq_iterator.Next();
  }

  while (itr.GetNext(path, ignored)) {
    ++path_count;

    vector<Item> lhs_spath(path.begin(), path.end());

    // add items with fquency 0 in rhs_frquency_table to this path and then sort it
    // using lhs_frquency_table.
    lhs_spath.insert(lhs_spath.end(), zero_freqs.begin(), zero_freqs.end());
    sort(lhs_spath.begin(), lhs_spath.end(), cmp_lhs);

    // check every connection in the path against lhs_conn_table. Remove it, if it
    // does not exist in the lhs_conn_table. (e.g. a->b->c => a->c (if a->b does not exist in lhs_conn_table))
    size_t size = lhs_spath.size();
    size_t i = 0;
    while (i < size - 1) {
      if (!lhs_conn_table->connectionExist(lhs_spath[i], lhs_spath[i + 1], lhs_frequency_table)) {
        lhs_spath.erase(lhs_spath.begin() + i + 1);
        size --;
        continue;
      }
      i ++;
    }

    // Sort the path in the current tree based on the RHS's frequency table.
    vector<Item> rhs_spath(path.begin(), path.end());
    sort(rhs_spath.begin(), rhs_spath.end(), cmp_rhs);

    int dist = edit_distance(lhs_spath, rhs_spath);
    if (dist) {
      instability += double(dist) / path.size();
    }
  }

  return instability / path_count;
}

// Minh's added codes

void sortAVGRanking(const vector<Item>& itemList, vector<float>& rankedFloatList, const ItemMapCmp<unsigned>& compared_LHS) {
  // read through the vector and rank it by double,
  // this means  1 2 3 4 5 where item 2 == item 3, to be ranked as 1 2.5 2.5 4 5
  int frequencyWithTheSameValue = 1;
  for (int i = 1; i < itemList.size(); ++i) {
    Item item1 = itemList.at(i - 1);
    Item item2 = itemList.at(i);
    if (compared_LHS.freq.Get(item1) == compared_LHS.freq.Get(item2)) {
      rankedFloatList[i] += rankedFloatList[i - 1];
      frequencyWithTheSameValue++;
    } else {
      float averageRanking = rankedFloatList[i - 1] / frequencyWithTheSameValue;
      for (int j = i - 1; j >= i - frequencyWithTheSameValue; --j) {
        rankedFloatList[j] = averageRanking;
      }
      frequencyWithTheSameValue = 1;
    }
    if (i == itemList.size() - 1 && frequencyWithTheSameValue > 1) {
      float averageRanking = rankedFloatList[i] / frequencyWithTheSameValue;
      for (int j = i; j >= i - frequencyWithTheSameValue + 1; --j) {
        rankedFloatList[j] = averageRanking;
      }
    }
  }
}

double StructuralStreamDriftDetector::TreeAutomaticThreshold(
  const ItemMap<unsigned>* lhs_frequency_table,
  const ItemMap<unsigned>* rhs_frequency_table) const {
  uint32_t path_count = 0;

  ItemMapCmp<unsigned> cmp_lhs(*lhs_frequency_table);
  ItemMapCmp<unsigned> cmp_rhs(*rhs_frequency_table);

  //ASSERT(cmp_lhs.freq.valid.size > cmp_rhs.freq.valid.size);

  // declare left and right list of item to be sorted and compare the index
  vector<Item> leftItems;
  vector<Item> rightItems;
  ItemMap<unsigned>::Iterator leftIter = lhs_frequency_table->GetIterator();
  ItemMap<unsigned>::Iterator rightIter = rhs_frequency_table->GetIterator();

  TreePathIterator itr(tree.get());
  vector<Item> path;
  unsigned ignored;
  while (itr.GetNext(path, ignored)) {
    ++path_count;
  }

  //push item to a vector of leftItems
  uint32_t n_value = 0;
  uint32_t maxLeftItempID = 0;
  while (leftIter.HasNext()) {
    n_value++;
    Item leftItem = leftIter.GetKey();
    if (maxLeftItempID <= leftItem.GetId())	{
      maxLeftItempID = leftItem.GetId();
    }

    leftItems.push_back(leftItem);
    leftIter.Next();
  }
  //push item to a vector of rightItems
  while (rightIter.HasNext()) {
    Item rightItem = rightIter.GetKey();
    //Only add the the compare list if item appears in both trees
    if (lhs_frequency_table->Contains(rightItem)) {
      rightItems.push_back(rightItem);
    }
    rightIter.Next();
  }

  // sort left and right array based on cmp_lhs and cmp_rhs
  sort(leftItems.begin(), leftItems.end(), cmp_lhs);
  sort(rightItems.begin(), rightItems.end(), cmp_rhs);

  // create ranking array and initialise them	(only compared with left item list)
  vector<float> leftRankedFloat(leftItems.size());
  vector<float> rightRankedFloat(leftItems.size());

  // initialise float ranking, it starts with 1.0 2.0 3.0 4.0 5.0 ....
  for (int i = 0; i < leftItems.size(); ++i) {
    leftRankedFloat[i] = i + 1;
    rightRankedFloat[i] = i + 1;
  }

  // calculate the float ranking, it may end up with sthing like 1.0 2.0 2.5 2.5 5.0 ...
  sortAVGRanking(leftItems, leftRankedFloat, cmp_lhs) ;
  sortAVGRanking(rightItems, rightRankedFloat, cmp_rhs) ;

  // create ranking array and initialise them all to zero
  vector<float> leftItemsWithRanks(maxLeftItempID + 1);
  vector<float> rightItemsWithRanks(maxLeftItempID + 1);
  for (int i = 0; i < maxLeftItempID + 1; ++i) {
    leftItemsWithRanks[i] = 0;
    rightItemsWithRanks[i] = 0;
  }

  for (int i = 0; i < maxLeftItempID; ++i)	{
    Item leftItem = leftItems.at(i);
    int mIDL = leftItem.GetId();
    leftItemsWithRanks[mIDL] = leftRankedFloat[i];
    Item rightItem = rightItems.at(i);
    int mIDR = rightItem.GetId();
    rightItemsWithRanks[mIDR] = rightRankedFloat[i];
  }

  // For calculating rho and e_cut
  double totalRankDifference = 0;
  for (int i = 1; i < maxLeftItempID + 1; ++i)	{
    totalRankDifference = totalRankDifference + (leftItemsWithRanks[i] - rightItemsWithRanks[i]) * (leftItemsWithRanks[i] - rightItemsWithRanks[i]);
  }

  double alpha = 0.01;
  double rho = 0.0;
  double z =  2.24;
  if (alpha == 0.01)
    z = 2.24;
  if (alpha == 0.005)
    z = 2.41;
  if (alpha == 0.1)
    z = 1.29;
  if(alpha == 0.25)
    z = 0.69;
  if (alpha == 0.001)
    z = 2.96;
  if (alpha == 0.05)
    z = 1.64;

  rho = z/sqrt(double(n_value - 1));


  //double rho = (6.0 * totalRankDifference / (n_value * (n_value * n_value - 1)));
  // e_cut = ((1-rho)(n-1)) / (6.|P|)		// |P| = path_count
  double e_cut = (1.0 - rho) * (n_value * n_value - 1) / (6 * n_value * path_count);
  //  ASSERT(e_cut >= 0);
  //Log("  Spearman's rank correlation coefficient rho = %f similarity (0->1)\n", (1 - rho));
  //Log("  Statistically calculated e_cut = %f\n", fabs(e_cut));
  return fabs(e_cut);
}


static double hoeffdingBound(int n0, int n1, double delta) {
  double m = 1.0 / ((1.0 / n0) + (1.0 / n1));
  double eta = 0.0;
  double bounds = (1.0 / 2 * m) * log(4 * (n0 + n1) / (delta));
  eta = sqrt(bounds);
  return eta;
}

double StructuralStreamDriftDetector::TableInstability(
  const ItemMap<unsigned>* other_table,
  uint32_t other_block_size) const {
  uint32_t data_set_size = data_set->NumTransactions();
  uint32_t unstable_count = 0;

  // Note: we exploit the fact that the current tree's frequency table
  // must be a super set of previous ones; whatever is in the previous
  // frequency table *must* be in the current one. So we iterate over
  // the current frequency table (rather than the previous block's one)
  // so that we don't miss keys in our iteration.
  ItemMap<unsigned>::Iterator itr = tree->FrequencyTable().GetIterator();
  uint32_t count = 0;
  while (itr.HasNext()) {
    count++;
    Item item = itr.GetKey();
    double change = fabs(double(itr.GetValue() - other_table->Get(item, 0)));
    double epsilon = hoeffdingBound(data_set_size,
                                    other_block_size,
                                    0.001);
    if (change > epsilon) {
      unstable_count++;
    }
    itr.Next();
  }
  return double(unstable_count) / count;
}

void StructuralStreamDriftDetector::MaybePrintBlocks(const std::string& aMsg) {
  if (!print_blocks) {
    return;
  }
  Log(aMsg.c_str());
  for (int i = 0; i < check_points.size(); i++) {
    Log("  Block %d [%d, %d]\n", i, check_points[i].start_tid, check_points[i].end_tid);
  }
}

void StructuralStreamDriftDetector::Add(Transaction& transaction) {
  tree->SortTransaction(transaction.items);
  tree->Insert(transaction.items);
  data_set->Append(transaction);

  // check point for every check_interval = block size
  uint32_t interval = adaptive_window ? aw_check_interval : check_interval;
  ASSERT(interval > 0);

  if (transaction.id != 0 && (transaction.id + 1) % interval == 0) {
    TransactionId start = transaction.id + 1 - interval;
    TransactionId end = transaction.id;
    tree->Sort();
    Log("\nCheck point for transactions [%lu,%lu]\n", start, end);
    if (almost_exact) {
      check_points.push_back(CheckPoint(start, end, tree->FrequencyTable(),
                                        ConnectionTable(tree->GetRoot(), check_points.size() > 0 ? &check_points[check_points.size() - 1].conn_table : NULL)));
    } else {
      check_points.push_back(CheckPoint(start, end, tree->FrequencyTable()));
    }

    if (adaptive_window) {
      arrangeCheckPoints(2);
    }

    Log("\nHave %d check points, a total of %d transactions in range [%u,%u]\n",
        check_points.size(),
        check_points.back().end_tid - check_points.front().start_tid,
        check_points.front().start_tid,
        check_points.back().end_tid);

    MaybePrintBlocks("Check points:\n");

    int unstable_index = FindLastUnstableCheckPoint();
    if (unstable_index != -1) {
      Log("Tree is unstable, mining for rules...\n");
      mining_context->Mine(tree.get(), data_set.get());
      Log("Purging blocks and check points before transaction %d\n",
          check_points[unstable_index].end_tid);
      PurgeBlocksUpTo(unstable_index);

      MaybePrintBlocks("Check points after purge:\n");

    } else {
      Log("Tree is stable, not mining for rules...\n");
    }
  }
}

void StructuralStreamDriftDetector::PurgeBlocksUpTo(int block_index) {
  // When we purge blocks we must subtract the frequency counts of the blocks
  // we're purging from the blocks that we're leaving behind, otherwise the
  // blocks left behind will have invalid frequency counts. Note that if
  // we do this properly, no item should have a negative frequency count.

  TransactionId end_tid = check_points[block_index].end_tid;
  while (data_set->NumTransactions() > 0 &&
         data_set->Front().id <= end_tid) {
    Transaction transaction = data_set->Front();
    tree->SortTransaction(transaction.items);
    tree->Remove(transaction.items);
    data_set->Pop();

    // Iterate over the non-purged blocks, and update their frequency tables
    // to reflect that the transaction has been removed.
    for (uint32_t check_point_idx = block_index + 1;
         check_point_idx < check_points.size();
         check_point_idx++) {
      ItemMap<unsigned>& frequency_table = check_points[check_point_idx].frequency_table;
      for (uint32_t item_idx = 0; item_idx < transaction.items.size(); item_idx++) {
        Item item = transaction.items[item_idx];
        ASSERT(frequency_table.Get(item) >= 1);
        frequency_table.Decrement(item, 1);
      }
    }
  }

  // Remove the purged check points.
  check_points.erase(check_points.begin(), check_points.begin() + block_index + 1);

  // remove old connections if almost-exact is enabled.
  if (almost_exact)
    for (uint32_t check_point_idx = 0; check_point_idx < check_points.size(); check_point_idx++) {
      check_points[check_point_idx].conn_table.purge(purgeThreshold);
    }

}

int StructuralStreamDriftDetector::FindLastUnstableCheckPoint() {
  Log("Checking instability of previous blocks...%s\n",
      ((check_points.size() > 1) ? "" : " no previous blocks yet..."));
  if (use_distribution_drift) {
    return FindUnstableCheckPointByDistributionChange();
  }
  else if (window_cmp) {
    return FindUnstableCheckPointByPartition();
  }
  else {
    return FindUnstableCheckPointByProgressive();
  }
}

uint32_t
StructuralStreamDriftDetector::SizeOfWindow(std::vector<CheckPoint>::const_iterator start,
                                            std::vector<CheckPoint>::const_iterator end)
{
  uint32_t sz = 0;
  for_each(start, end, [&](const CheckPoint& checkPoint) {
      sz += checkPoint.Size();
  });
  return sz;
}

static double
Variance(const uint32_t count, const uint32_t N)
{
  // Variance is defined as 1/N * E[(x-mu)^2]. We consider our X to be a
  // stream of N instances of [0,1] values; 1 if item appears in a transaction,
  // 0 if not. We know that the average is count/N, and that X is 1
  // count times, and 0 (N-count) times, so the variance then becomes:
  // 1/N * (count * (1 - support)^2 + (N-count) * (0 - support)^2).
  double support = (double)count / N;
  return (count * pow(1-support, 2) + (N - count) * pow(0-support, 2)) / N;
}

int
StructuralStreamDriftDetector::FindUnstableCheckPointByDistributionChange()
{
  if (check_points.size() == 1) {
    return -1;
  }

  // Accumulate all the check point's frequency tables, so we know all items
  // in the window.
  ItemMap<unsigned> items;
  for (const CheckPoint& checkPoint : check_points) {
    items.Add(checkPoint.frequency_table);
  }

  ItemMap<unsigned> lhs(items);
  ItemMap<unsigned> rhs;

  int32_t block_index = check_points.size() - 2;
  while (block_index > 0) {

    lhs.Remove(check_points[block_index + 1].frequency_table);
    rhs.Add(check_points[block_index + 1].frequency_table);

    auto begin = check_points.begin();
    int32_t n_lhs = SizeOfWindow(begin, begin + block_index);
    int32_t n_rhs = SizeOfWindow(begin + block_index, check_points.end());
    uint32_t n = n_lhs + n_rhs;
    ASSERT(n == SizeOfWindow(begin, check_points.end()));

    for (auto itr = items.GetIterator(); itr.HasNext(); itr.Next()) {
      Item item = itr.GetKey();
      uint32_t u_lhs = lhs.Get(item);
      uint32_t u_rhs = rhs.Get(item);
      double v_lhs = Variance(u_lhs, n_lhs);
      double v_rhs = Variance(u_rhs, n_rhs);
      double v = Variance(items.Get(item), n);

      double absValue = u_lhs / n_lhs - u_rhs / n_rhs;
      const double mintMinWinLength = 5; // value copied from adWin.java
      double dd = log(2 * log(n) / dbdd_delta);
      double m = ((double)1 / ((n_rhs - mintMinWinLength + 1))) +
                 ((double)1 / ((n_lhs - mintMinWinLength + 1)));
      double epsilon = sqrt(2 * m * v * dd) + (double)2 / 3 * dd * m;

      bool shouldCut = abs(absValue) > epsilon;
      if (shouldCut) {
        return block_index;
      }
    }
    block_index--;
  }
  return -1;
}

int
StructuralStreamDriftDetector::FindUnstableCheckPointByProgressive() {
  Log("  Finding unstable cut point using progressive method\n");

  // Blocks are stored most recent last. So traverse backwards through the
  // vector (most recent to oldest) and look for the point where we become
  // "unstable", and drop all blocks up to and including the unstable block.
  int32_t block_index = int(check_points.size()) - 2;
  double prev_tree_instability = std::numeric_limits<double>::infinity();
  double prev_table_instability = std::numeric_limits<double>::infinity();
  while (block_index >= 0) {
    const CheckPoint& check_point = check_points[block_index];
    bool must_merge = false;
    if (have_structure_drift_threshold) {
      double tree_instability = TreeInstability(&check_point.frequency_table);
      if (print_blocks) {
        Log("  Block [%d,%d] has structural instability=%lg\n",
            check_point.start_tid, check_point.end_tid, tree_instability);
      }
      if (tree_instability > structure_drift_threshold) {
        Log("  Block considered %d [%d-%d] unstable by structure drift threshold check.\n",
            block_index,
            check_point.start_tid, check_point.end_tid);
        return block_index;
      }
      if (have_structure_merge_threshold) {
        if (fabs(tree_instability - prev_tree_instability) < structure_merge_threshold) {
          const CheckPoint& next = check_points[block_index + 1];
          Log("  Blocks [%d,%d] and [%d,%d] have similar tree stability, will merge if not purged\n",
              check_point.start_tid, check_point.end_tid,
              next.start_tid, next.end_tid);
          must_merge = true;
        }
        prev_tree_instability = tree_instability;
      }
    }
    if (have_item_drift_threshold) {
      uint32_t block_size = check_point.end_tid - check_point.start_tid;
      double table_instability =
        TableInstability(&check_point.frequency_table, block_size);
      if (print_blocks) {
        Log("  Block [%d,%d] has item frequency table instability=%lg\n",
            check_point.start_tid, check_point.end_tid, table_instability);
      }
      if (table_instability > item_drift_threshold) {
        Log("  Block considered %d [%d-%d] unstable by item drift threshold check.\n",
            block_index,
            check_point.start_tid, check_point.end_tid);
        return block_index;
      }
      if (have_item_freq_merge_thresold) {
        if (fabs(table_instability - prev_table_instability) < item_freq_merge_threshold) {
          const CheckPoint& next = check_points[block_index + 1];
          Log("  Blocks [%d,%d] and [%d,%d] have similar item frequency tables, will merge if not purged\n",
              check_point.start_tid, check_point.end_tid,
              next.start_tid, next.end_tid);
          must_merge = true;
        }
        prev_table_instability = table_instability;
      }
    }
    if (must_merge) {
      MergeCheckPointWithNext(block_index);

      // Block is too similar to the one after it. Merge the two blocks
      // together.
    }
    block_index--;
  }
  return -1;
}

void
StructuralStreamDriftDetector::MergeCheckPointWithNext(int block_index) {
  // Block is too similar to the one after it. Merge the two blocks
  // together.
  //ASSERT(block_index != int(check_points.size()) - 2);
  CheckPoint& block = check_points[block_index];
  CheckPoint& next = check_points[block_index + 1];

  Log("  Merging Blocks [%d,%d] and [%d,%d]\n",
      block.start_tid, block.end_tid,
      next.start_tid, next.end_tid);

  block.end_tid = next.end_tid;

  // Merge the items in |next|'s frequency table into the block's
  // frequency table.
  block.frequency_table.Clear();
  block.frequency_table.Add(next.frequency_table);
  block.conn_table = next.conn_table;

  check_points.erase(check_points.begin() + block_index + 1);
}

int
StructuralStreamDriftDetector::FindUnstableCheckPointByPartition() {
  double RATIO_BETWEEN_E_CUT_E_WANRINGS = 0.8;
  //  Log("  Finding unstable cut point using partition method\n");

  // Blocks are stored most recent last. So traverse backwards through the
  // vector (most recent to oldest) and partition the data set representation
  // in two halves. When we find the partition point where the datasets differ,
  // we'll drop the older blocks.
  int last_block_index = int(check_points.size()) - 1;

  // Fill the left hand side of the partition with the frequency table
  // of the oldest check point.
  ItemMap<unsigned> lhs(check_points[0].frequency_table);
  ConnectionTable lhs_conn_table = check_points[0].conn_table;

  // Fill the right hand side of the partition with the frequency tables
  // of all checkpoints after the oldest check point.

  ItemMap<unsigned> rhs(check_points[last_block_index].frequency_table);
  //  rhs.Remove(lhs);
  ConnectionTable rhs_conn_table = check_points[last_block_index].conn_table;

  //double tree_instability = TreeInstability(&check_points[last_block_index].frequency_table);
  //Log("  Tree instability=%lg\n", tree_instability);

  int32_t block_index = 0;
  while (block_index < last_block_index && check_points.size() > 1) {
    const double instability = almost_exact ? TreeInstability(&lhs, &rhs, &lhs_conn_table, &rhs_conn_table) : TreeInstability(&lhs, &rhs);

   Log("  TreeInstability(&lhs, &rhs) instability=%lg\n", instability);
    /*
    Minh's code 2014/08/11
    */
    //if there is

    //only allow one to be set true
    ASSERT(have_automatic_ssdd_structural_drift_threshold != have_structure_drift_threshold);

    if (have_automatic_ssdd_structural_drift_threshold) {
      /*Log("  Calculate the automatic_drift_threshold at partition %d of %d blocks tids=[%d,%d]-[%d,%d]\n",
          block_index, last_block_index,
          check_points[0].start_tid,
          check_points[block_index].end_tid,
          check_points[block_index + 1].start_tid,
          check_points[last_block_index].end_tid);*/

      //calculate automatic_ssdd_structural_drift_threshold here
      automatic_ssdd_structural_drift_threshold =  TreeAutomaticThreshold(&lhs, &rhs); // = e_cut

      ASSERT(automatic_ssdd_structural_drift_threshold >= 0);

      automatic_ssdd_structural_merge_threshold = RATIO_BETWEEN_E_CUT_E_WANRINGS * automatic_ssdd_structural_drift_threshold;

      ASSERT(automatic_ssdd_structural_merge_threshold >= 0);

      if (instability > automatic_ssdd_structural_drift_threshold) {
        Log("  Partition instability=%lg\n", instability);
        if (almost_exact) {
          Log("\n lhs_conn_table:\n");
          lhs_conn_table.print();
          Log("\n rhs_conn_table:\n");
          rhs_conn_table.print();
        }
        return block_index;
      }

    } else if (have_structure_drift_threshold && instability > structure_drift_threshold) {
      Log("  Unstable at partition %d of %d blocks tids=[%d,%d]-[%d,%d]\n",
          block_index, last_block_index,
          check_points[0].start_tid,
          check_points[block_index].end_tid,
          check_points[block_index + 1].start_tid,
          check_points[last_block_index].end_tid);
      Log("  Partition instability=%lg\n", instability);
      return block_index;
    }


    // Move the item frequency counts from the first block in the RHS to
    // the LHS of the partition, so the next iteration compares the correct
    // frequency tabls. We need to do this regardless of whether we're
    // merging the blocks below.

    lhs.Clear();
    lhs.Add(check_points[block_index + 1].frequency_table);
    lhs_conn_table = check_points[block_index + 1].conn_table;

    //    rhs.Clear();
    //    rhs.Add(check_points[last_block_index].frequency_table);
    //    rhs_conn_table = check_points[last_block_index].conn_table;
    //    rhs.Remove(lhs);


    if (!adaptive_window) {
      // if (abs(tree_instability - prev_tree_instability) < structure_merge_threshold
      if (have_structure_merge_threshold && instability < structure_merge_threshold) {
        // Merge the block with the next. Note we don't increment block_index,
        // so that we'll re-run the checks on this block, since it's been merged
        // with another and its relative frequencies may have changed.
        MergeCheckPointWithNext(block_index);
      }
      // if having have_automatic_ssdd_structural_drift_threshold, use calculated merge_threshold value
      // if (abs(tree_instability - prev_tree_instability) < automatic_ssdd_structural_merge_threshold
      else if (have_automatic_ssdd_structural_drift_threshold && instability < automatic_ssdd_structural_merge_threshold) {
        // Merge the block with the next. Note we don't increment block_index,
        // so that we'll re-run the checks on this block, since it's been merged
        // with another and its relative frequencies may have changed.
        MergeCheckPointWithNext(block_index);
      } else {
        block_index++;
      }
      last_block_index = check_points.size() - 1;
    } else {
      block_index++;
    }
  }

  return -1;
}

size_t StructuralStreamDriftDetector::capacity(CheckPoint& c) {
  return (c.end_tid == 0) ? 0 : (size_t)((c.end_tid - c.start_tid + 1) / aw_check_interval);
}

void StructuralStreamDriftDetector::arrangeCheckPoints(int m) {
  int32_t idx = check_points.size() - m - 1;
  size_t exp_cap = 1;

  while ((idx >= 0) && (capacity(check_points[idx]) == exp_cap)) {
    MergeCheckPointWithNext(idx);
    idx -= m;
    exp_cap *= 2;
  }
}

// static
void StructuralStreamDriftDetector::Test() {
  vector<Item> v1(ToItemVector("a,b,c"));
  vector<Item> v2(ToItemVector("a,b,c"));
  ASSERT(edit_distance(v1, v2) == 0);

  ASSERT(edit_distance(ToItemVector("a,b,c,d"), ToItemVector("a,b,d,e")) == 2);
  ASSERT(edit_distance(ToItemVector("a,b,c,d"), ToItemVector("a,b,d")) == 1);
}

void StructuralStreamDriftDetector::TestSpearman() {

  vector<Item> testV1(ToItemVector("0,1,2,3,4,5"));
  vector<Item> testV2(ToItemVector("0,1,2,3,4,5"));
  int maxLeftItempID = 6;
  ASSERT(edit_distance(testV1, testV2) == 0);

  ItemMap<unsigned>* tst_lhs_frequency_table = new ItemMap<unsigned>();
  tst_lhs_frequency_table->Set(Item("0"), 1);
  tst_lhs_frequency_table->Set(Item("1"), 5);
  tst_lhs_frequency_table->Set(Item("2"), 6);
  tst_lhs_frequency_table->Set(Item("3"), 5);
  tst_lhs_frequency_table->Set(Item("4"), 5);
  tst_lhs_frequency_table->Set(Item("5"), 6);
  ItemMap<unsigned>* tst_rhs_frequency_table = new ItemMap<unsigned>();
  tst_rhs_frequency_table->Set(Item("0"), 1);
  tst_rhs_frequency_table->Set(Item("1"), 9);
  tst_rhs_frequency_table->Set(Item("2"), 11);
  tst_rhs_frequency_table->Set(Item("3"), 8);
  tst_rhs_frequency_table->Set(Item("4"), 8);
  tst_rhs_frequency_table->Set(Item("5"), 10);

  ItemMapCmp<unsigned> cmp_lhs(*tst_lhs_frequency_table);
  ItemMapCmp<unsigned> cmp_rhs(*tst_rhs_frequency_table);

  std::sort(testV1.begin(), testV1.end(), cmp_lhs);
  std::sort(testV2.begin(), testV2.end(), cmp_rhs);

  vector<float> leftRankedFloat(testV1.size());
  vector<float> rightRankedFloat(testV2.size());

  for (int i = 0; i < testV1.size(); i++) {
    leftRankedFloat[i] = i + 1;
    rightRankedFloat[i] = i + 1;
  }

  sortAVGRanking(testV1, leftRankedFloat, cmp_lhs);
  sortAVGRanking(testV2, rightRankedFloat, cmp_rhs);

  // create ranking array and initialise them all to zero
  vector<float> leftItemsWithRanks(maxLeftItempID + 1);
  vector<float> rightItemsWithRanks(maxLeftItempID + 1);
  for (int i = 0; i < maxLeftItempID + 1; ++i) {
    leftItemsWithRanks[i] = 0;
    rightItemsWithRanks[i] = 0;
  }

  for (int i = 0; i < maxLeftItempID; ++i)	{
    Item leftItem = testV1.at(i);
    int mIDL = leftItem.GetId();
    leftItemsWithRanks[mIDL] = leftRankedFloat[i];
    Item rightItem = testV2.at(i);
    int mIDR = rightItem.GetId();
    rightItemsWithRanks[mIDR] = rightRankedFloat[i];
  }

  for (int i = 1; i < maxLeftItempID + 1; ++i)	{
    //Item item1 = testV1.at(i);
    printf("Left Item %d is ranked at %f \n", i, leftItemsWithRanks[i]);
  }
  printf("----\n");
  for (int i = 1; i < maxLeftItempID + 1; ++i)	{
    //Item item2 = testV2.at(i);
    printf("Right Item %d is ranked at %f \n", i, rightItemsWithRanks[i]);
  }

  // For calculating rho
  double totalRankDifference = 0;
  for (int i = 1; i < maxLeftItempID + 1; ++i)	{
    totalRankDifference = totalRankDifference + (leftItemsWithRanks[i] - rightItemsWithRanks[i]) * (leftItemsWithRanks[i] - rightItemsWithRanks[i]);
  }
#ifdef DEBUG
  int counter = 6;
  double rho = 1.0 - (6.0 * totalRankDifference / (counter * (counter * counter - 1)));
  //calculated in excel by hand, RHO = 0.942857
  ASSERT(abs(rho - 0.942857) < 0.001);
#endif
}

