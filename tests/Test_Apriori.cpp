#include "gtest/gtest.h"
#include "AprioriFilter.h"

#include <iostream>
#include <string>

using namespace std;

// aExpected is itemsets, space delimted, and item inside the set ',' delimited.
bool CheckSet(string aExpected, set<ItemSet> aObserved) {
  vector<string> itemsets;
  Tokenize(aExpected, itemsets, " ");

  ASSERT(itemsets.size() == aObserved.size());
  if (itemsets.size() != aObserved.size()) {
    return false;
  }

  set<ItemSet>::const_iterator itr = aObserved.begin();
  set<ItemSet>::const_iterator end = aObserved.end();
  int i = 0;
  for (itr; itr != end; itr++, i++) {
    vector<string> items;
    Tokenize(itemsets[i], items, ",");
    ItemSet item1;
    for (unsigned i = 0; i < items.size(); i++) {
      item1 += Item(items[i]);
    }
    ItemSet item2(*itr);
    ASSERT(item1 == item2);
    if (!(item1 == item2)) {
      return false;
    }
  }
  return true;
}

extern void GenerateCandidates(const set<ItemSet>& aCandidates,
                               set<ItemSet>& aResult,
                               int aItemSetSize,
                               AprioriFilter* aFilter,
                               int numThreads);
extern void GenerateInitialCandidates(const InvertedDataSetIndex& aIndex,
                                      AprioriFilter& aFilter,
                                      set<ItemSet>& aCandidates);
extern bool ContainsAllSubSets(const set<ItemSet>& aContainer,
                               const ItemSet& aItemSet);

TEST(Apriori_Test, main) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  Item::ResetBaseId();

  {
    InvertedDataSetIndex index("datasets/test/test3.data");
    ASSERT_TRUE(index.Load());
    ASSERT_TRUE(index.GetNumItems() == 20);

    // Test GenerateInitialCandidates().
    {
      set<ItemSet> candidates;
      MinSupportFilter msf(0.6, index);
      GenerateInitialCandidates(index, msf, candidates);
      ASSERT_TRUE(CheckSet("a s u", candidates));
    }
    {
      set<ItemSet> candidates;
      MinSupportFilter msf(0.3, index);
      GenerateInitialCandidates(index, msf, candidates);
      // Candidates are: {a,c,d,e,f,g,r,s,t,u,w,y}
      CheckSet("a c d e f g r s t u w y", candidates);
    }

    // Test bool ContainsAllSubSets(const set<ItemSet> &aContainer, const ItemSet &aItemSet)
    {
      set<ItemSet> container;
      container.insert(ItemSet("a", "b"));
      container.insert(ItemSet("a", "c"));
      container.insert(ItemSet("b", "c"));

      ItemSet abc("a", "b", "c");
      ASSERT_TRUE(ContainsAllSubSets(container, abc));
    }
    // Test GenerateCandidates()...
    {
      set<ItemSet> P;
      set<ItemSet> R;
      AlwaysAcceptFilter F;

      ASSERT_TRUE(IntersectionSize(ItemSet("1", "2", "3"), ItemSet("1", "2", "4")) == 2);
      ASSERT_TRUE(IntersectionSize(ItemSet("1", "2", "4"), ItemSet("1", "3", "4")) == 2);
      ASSERT_TRUE(IntersectionSize(ItemSet("1", "3", "4"), ItemSet("1", "3", "5")) == 2);
      ASSERT_TRUE(IntersectionSize(ItemSet("1", "2", "3"), ItemSet("2", "3", "4")) == 2);


      P.insert(ItemSet("1", "2", "3"));
      P.insert(ItemSet("1", "2", "4"));
      P.insert(ItemSet("1", "3", "4"));
      P.insert(ItemSet("1", "3", "5"));
      P.insert(ItemSet("2", "3", "4"));

      GenerateCandidates(P, R, 4, &F, 1);

      ASSERT_TRUE(R.size() == 1);
      ASSERT_TRUE(*R.begin() == ItemSet("1", "2", "3", "4"));
    }
  }
  {
    // Test a simulation of Apriori, with the data set given by Agrual's paper.
    InvertedDataSetIndex index("datasets/test/test4.data");
    ASSERT_TRUE(index.Load());

    /*
      Data set:
      1 3 4
      2 3 5
      1 2 3 5
      2 5

      Initial candidates / Support
      1/2
      2/3
      3/3
      5/3
    */
    int c = 0;
    ASSERT_TRUE((c = index.Count(ItemSet("1"))) == 2);
    ASSERT_TRUE((c = index.Count(ItemSet("2"))) == 3);
    ASSERT_TRUE((c = index.Count(ItemSet("3"))) == 3);
    ASSERT_TRUE((c = index.Count(ItemSet("5"))) == 3);

    MinCountFilter filter(2, index);
    set<ItemSet> initialCandidates;
    GenerateInitialCandidates(index, filter, initialCandidates);
    ASSERT_TRUE(CheckSet("1 2 3 5", initialCandidates));

    //Test next generation...
    set<ItemSet> nextGen;
    GenerateCandidates(initialCandidates, nextGen, 2, &filter, 1);
    ASSERT_TRUE(CheckSet("1,3 2,3 2,5 3,5", nextGen));

    set<ItemSet> tmp = nextGen;
    nextGen.clear();
    GenerateCandidates(tmp, nextGen, 3, &filter, 1);
    ASSERT_TRUE(CheckSet("2,3,5", nextGen));

  }
}

extern int MinAbsSup(int A, int B, int N, double E);

TEST(MinAbsSupFilter, main) {
  EXPECT_EQ(MinAbsSup(250, 250, 1000, 0.999), 81);
  EXPECT_EQ(MinAbsSup(250, 500, 1000, 0.999), 146);
  EXPECT_EQ(MinAbsSup(600, 500, 1000, 0.999), 324);
  EXPECT_EQ(MinAbsSup(500, 500, 1000, 0.999), 274);
  EXPECT_EQ(MinAbsSup(500, 1000, 1000, 0.999), 500);
  EXPECT_EQ(MinAbsSup(500, 1000, 10000, 0.999), 71);
  EXPECT_EQ(MinAbsSup(50, 1000, 10000, 0.999), 12);
  EXPECT_EQ(MinAbsSup(50, 5000, 10000, 0.999), 36);
}
