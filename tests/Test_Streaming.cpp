#include "gtest/gtest.h"

#include "Item.h"
#include "FPNode.h"
#include "Utils.h"
#include <vector>

using namespace std;

extern int edit_distance(const vector<Item>& v1, const vector<Item>& v2);

extern void sortAVGRanking(const vector<Item>& itemList,
                           vector<float>& rankedFloatList,
                           const ItemMapCmp<unsigned>& compared_LHS);

TEST(StructuralStreamDriftDetector, EditDistance) {
  vector<Item> v1(ToItemVector("a,b,c"));
  vector<Item> v2(ToItemVector("a,b,c"));
  EXPECT_EQ(edit_distance(v1, v2), 0);

  EXPECT_EQ(edit_distance(ToItemVector("a,b,c,d"), ToItemVector("a,b,d,e")), 2);
  EXPECT_EQ(edit_distance(ToItemVector("a,b,c,d"), ToItemVector("a,b,d")), 1);
}

TEST(StructuralStreamDriftDetector, Spearman) {
  Item::ResetBaseId();

  vector<Item> testV1(ToItemVector("0,1,2,3,4,5"));
  vector<Item> testV2(ToItemVector("0,1,2,3,4,5"));
  int maxLeftItempID = 6;
  EXPECT_EQ(edit_distance(testV1, testV2), 0);

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

  for (size_t i = 0; i < testV1.size(); i++) {
    leftRankedFloat[i] = float(i + 1);
    rightRankedFloat[i] = float(i + 1);
  }

  sortAVGRanking(testV1, leftRankedFloat, cmp_lhs);
  sortAVGRanking(testV2, rightRankedFloat, cmp_rhs);

  // create ranking array and initialize them all to zero
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

