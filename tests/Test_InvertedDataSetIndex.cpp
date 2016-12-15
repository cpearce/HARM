#include "gtest/gtest.h"
#include "InvertedDataSetIndex.h"
#include "TidList.h"
#include "WindowIndex.h"
#include "VariableWindowDataSet.h"

using namespace std;

TEST(InvertedDataSetIndex, main) {
  {
    InvertedDataSetIndex index("NotARealFileName");
    ASSERT_TRUE(!index.Load());
  }
  {
    /*
      Test: Dataset 1, check loading, test supports.
      a,b,c,d,e,f
      g,h,i,j,k,l
      z,x
      z,x
      z,x,y
      z,x,y,i
    */
    InvertedDataSetIndex index("datasets/test/test1.data");
    ASSERT_TRUE(index.Load());
    ASSERT_TRUE(index.Count(Item("a")) == 1);
    ASSERT_TRUE(index.Count(Item("b")) == 1);
    ASSERT_TRUE(index.Count(Item("c")) == 1);
    ASSERT_TRUE(index.Count(Item("d")) == 1);
    ASSERT_TRUE(index.Count(Item("e")) == 1);
    ASSERT_TRUE(index.Count(Item("f")) == 1);
    ASSERT_TRUE(index.Count(Item("h")) == 1);
    ASSERT_TRUE(index.Count(Item("i")) == 2);
    ASSERT_TRUE(index.Count(Item("j")) == 1);
    ASSERT_TRUE(index.Count(Item("k")) == 1);
    ASSERT_TRUE(index.Count(Item("l")) == 1);
    ASSERT_TRUE(index.Count(Item("z")) == 5);
    ASSERT_TRUE(index.Count(Item("x")) == 4);
    ASSERT_TRUE(index.Count(Item("y")) == 2);

    int sup_zx = index.Count(ItemSet("z", "x"));
    ASSERT_TRUE(sup_zx == 4);

    int sup_zxy = index.Count(ItemSet("z", "x", "y"));
    ASSERT_TRUE(sup_zxy == 2);

    int sup_zxyi = index.Count(ItemSet("z", "x", "y", "i"));
    ASSERT_TRUE(sup_zxyi == 1);

    // Test InvertedDataSetIndex.GetItems().
    const char* test1Items[] =
    {"a", "b", "c", "d", "e", "f", "h", "i", "j", "k", "l", "z", "x", "y"};
    const set<Item>& items = index.GetItems();
    const int numItems = sizeof(test1Items) / sizeof(const char*);
    ASSERT_TRUE(numItems == 14);
    ASSERT_TRUE(items.size() == numItems);
    set<Item>::const_iterator itr = items.begin();
    ASSERT_TRUE((itr = items.find(Item("NotAnItem"))) == items.end());
    ASSERT_TRUE((itr = items.find(Item("AlsoNotAnItem"))) == items.end());
    for (int i = 0; i < numItems; i++) {
      ASSERT_TRUE((itr = items.find(Item(test1Items[i]))) != items.end());
    }
  }
  {
    // Test partially dirty data, tests parsing/tokenizing.
    /*
      aa, bb, cc , dd
      aa, gg,
      aa, bb
    */
    InvertedDataSetIndex index1("datasets/test/test2.data");
    ASSERT_TRUE(index1.Load());
    ASSERT_TRUE(index1.Count(Item("aa")) == 3);
    ASSERT_TRUE(index1.Count(ItemSet("aa", "bb")) == 2);
    ASSERT_TRUE(index1.Count(Item("gg")) == 1);
    ASSERT_TRUE(index1.Count(ItemSet("bollocks", "nothing")) == 0);

    // Test IntersectionSize().
    // int IntersectionSize(const ItemSet& aItem1, const ItemSet& aItem2)
    ItemSet xyz("x", "y", "z");
    ItemSet wxy("w", "x", "y");
    ASSERT_TRUE(IntersectionSize(xyz, wxy) == 2);
    ASSERT_TRUE(IntersectionSize(wxy, xyz) == 2);

    const char* data_abxusji[] = {"a", "b", "x", "u", "s", "j", "i"};
    ItemSet abxusji(data_abxusji, sizeof(data_abxusji) / sizeof(const char*));
    const char* data_ndlashyilw[] = {"n", "d", "l", "a", "s", "h", "y", "i", "l", "w"};
    ItemSet ndlashyilw(data_ndlashyilw, sizeof(data_ndlashyilw) / sizeof(const char*));
    ASSERT_TRUE(IntersectionSize(abxusji, ndlashyilw) == 3);
  }

  {
    InvertedDataSetIndex index1("datasets/test/UCI-zoo.csv");
    // hair = 1 count == 43
    ASSERT_TRUE(index1.Load());
    ASSERT_TRUE(index1.Count(Item("hair=1")) == 43);
    ASSERT_TRUE(index1.Count(ItemSet("airbourne=0", "backbone=1", "venomous=0")) == 61);

  }

  {
    /*
      aa, bb, cc , dd
      aa, gg,
      aa, bb
    */
    InvertedDataSetIndex index1("datasets/test/test2.data");
    ASSERT_TRUE(index1.Load());
    ASSERT_TRUE(index1.Count(Item("aa")) == 3);
    ASSERT_TRUE(index1.Count(ItemSet("aa", "bb")) == 2);
    ASSERT_TRUE(index1.Count(Item("gg")) == 1);
    ASSERT_TRUE(index1.Count(ItemSet("bollocks", "nothing")) == 0);
    ASSERT_TRUE(index1.NumTransactions() == 3);
  }
}

TEST(TidList_Test, main) {
  TidList tl;
  tl.Set(123, true);
  ASSERT_TRUE(tl.Get(123));
  tl.Set(123, false);
  ASSERT_FALSE(tl.Get(123));

  tl.Set(31, true);
  tl.Set(32, true);
  tl.Set(33, true);
  ASSERT_TRUE(tl.Get(31));
  ASSERT_TRUE(tl.Get(32));
  ASSERT_TRUE(tl.Get(33));

  tl.Set(31, false);
  ASSERT_FALSE(tl.Get(31));
  ASSERT_TRUE(tl.Get(32));
  ASSERT_TRUE(tl.Get(33));

  tl.Set(32, false);
  ASSERT_FALSE(tl.Get(31));
  ASSERT_FALSE(tl.Get(32));
  ASSERT(tl.Get(33));

  tl.Set(33, false);
  ASSERT_FALSE(tl.Get(31));
  ASSERT_FALSE(tl.Get(32));
  ASSERT_FALSE(tl.Get(33));
}

TEST(CoocurrenceGraph, main) {
  Item::SetCompareMode(Item::INSERTION_ORDER_COMPARE);

  CoocurrenceGraph g;
  for (int i = 0; i < 4; i++) {
    g.Increment({Item("a"), Item("b")});
  }
  g.Increment({Item("b"), Item("a")});

  g.Increment({Item("c"), Item("a")});
  g.Increment({Item("a"), Item("c")});

  g.Increment({Item("c"), Item("b")});

  g.Increment({Item("c"), Item("d")});
  g.Increment({Item("c"), Item("d")});

  int c = 0;

  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("b")), 5);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("c")), 2);
  EXPECT_EQ(g.CoocurrenceCount(Item("c"), Item("b")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("c"), Item("d")), 2);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("d")), 0);

  EXPECT_EQ(g.CoocurrenceCount(Item("h"), Item("j")), 0);

  g.Increment(ItemSet("h", "j", "k").AsVector());
  g.Increment(ItemSet("a", "c", "k").AsVector());

  EXPECT_EQ(g.CoocurrenceCount(Item("h"), Item("j")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("j"), Item("k")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("h"), Item("k")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("k")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("c")), 3);

  g.Decrement(ItemSet("h", "j", "k").AsVector());

  EXPECT_EQ(g.CoocurrenceCount(Item("h"), Item("j")), 0);
  EXPECT_EQ(g.CoocurrenceCount(Item("j"), Item("k")), 0);
  EXPECT_EQ(g.CoocurrenceCount(Item("h"), Item("k")), 0);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("k")), 1);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("c")), 3);
  EXPECT_EQ(g.CoocurrenceCount(Item("c"), Item("k")), 1);

  vector<Item> n;
  g.GetNeighbourhood(Item("a"), n);
  EXPECT_EQ(ItemSet("b", "c", "k"), n);

  g.Decrement(ItemSet("a", "c", "k").AsVector());
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("k")), 0);
  EXPECT_EQ(g.CoocurrenceCount(Item("a"), Item("c")), 2);
  EXPECT_EQ(g.CoocurrenceCount(Item("c"), Item("k")), 0);

  n.clear();
  g.GetNeighbourhood(Item("a"), n);
  EXPECT_EQ(ItemSet("b", "c"), n);
}


TEST(WindowIndex, main) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  {
    WindowIndex index("datasets/test/test1.data", NULL, 10, 1);
    bool loaded = index.Load();
    EXPECT_EQ(index.Count(Item("a")), 1);
    EXPECT_EQ(index.Count(Item("b")), 1);
    EXPECT_EQ(index.Count(Item("c")), 1);
    EXPECT_EQ(index.Count(Item("d")), 1);
    EXPECT_EQ(index.Count(Item("e")), 1);
    EXPECT_EQ(index.Count(Item("f")), 1);
    EXPECT_EQ(index.Count(Item("h")), 1);
    EXPECT_EQ(index.Count(Item("i")), 2);
    EXPECT_EQ(index.Count(Item("j")), 1);
    EXPECT_EQ(index.Count(Item("k")), 1);
    EXPECT_EQ(index.Count(Item("l")), 1);
    EXPECT_EQ(index.Count(Item("z")), 5);
    EXPECT_EQ(index.Count(Item("x")), 4);
    EXPECT_EQ(index.Count(Item("y")), 2);

    int sup_zx = index.Count(ItemSet("z", "x"));
    EXPECT_EQ(sup_zx, 4);

    int sup_zxy = index.Count(ItemSet("z", "x", "y"));
    EXPECT_EQ(sup_zxy, 2);

    int sup_zxyi = index.Count(ItemSet("z", "x", "y", "i"));
    EXPECT_EQ(sup_zxyi, 1);
    EXPECT_TRUE(loaded);
  }
  {
    WindowIndex index("datasets/test/test1.data", NULL, 2, 1);
    bool loaded = index.Load();
    EXPECT_TRUE(loaded);
  }
  {
    // Test partially dirty data, tests parsing/tokenizing.
    /*
      aa, bb, cc , dd
      aa, gg,
      aa, bb
    */
    WindowIndex index1("datasets/test/test2.data", NULL, 3, 1);
    EXPECT_TRUE(index1.Load());
    EXPECT_EQ(index1.Count(Item("aa")), 3);
    EXPECT_EQ(index1.Count(ItemSet("aa", "bb")), 2);
    EXPECT_EQ(index1.Count(Item("gg")), 1);
    EXPECT_EQ(index1.Count(ItemSet("bollocks", "nothing")), 0);
  }

  {
    WindowIndex index1("datasets/test/UCI-zoo.csv", NULL, 101, 1);
    // hair = 1 count == 43
    EXPECT_TRUE(index1.Load());
    EXPECT_EQ(index1.Count(Item("hair=1")), 43);
    EXPECT_EQ(index1.Count(ItemSet("airbourne=0", "backbone=1", "venomous=0")), 61);
  }
  #ifdef INDEX_STRESS_TEST
  {
    WindowIndex index("datasets/test/kosarak.csv", NULL, 1, 1);
    bool loaded = index.Load();
    EXPECT_TRUE(loaded);
  }
  {
    WindowIndex index("datasets/test/kosarak.csv", NULL, 500, 1);
    bool loaded = index.Load();
    EXPECT_TRUE(loaded);
  }
  {
    WindowIndex index("datasets/test/kosarak.csv", NULL, 50000, 1);
    bool loaded = index.Load();
    EXPECT_TRUE(loaded);
  }
  #endif
}

// Makes a Transaction for testing.
// Transaction have their tid=n.
// All Transactions contain Item("a").
// Transactions with tid divisible by 2 contain Item("b").
// Transactions with tid divisible by 3 contain Item("c").
static Transaction MakeTestTransaction(uint32_t n) {
  string items("a");
  if ((n % 2) == 0) {
    items += ",b";
  }
  return Transaction(n, items);
}

// Returns the number of transactions which contain Item("a") in the range
// [start_id, start_id+len]. Note [0,0] is a length 1 range.
static uint32_t TestExpectedItemA(uint32_t start_id, uint32_t len) {
  return len;
}

// Returns the number of transactions which contain Item("b") in the range
// [start_id, end_id].
static uint32_t TestExpectedItemB(uint32_t start_id, uint32_t len) {
  uint32_t count = 0;
  for (uint32_t i = start_id; i < start_id + len; i++) {
    if ((i % 2) == 0) {
      count++;
    }
  }
  return count;
}

TEST(VariableWindowDataSet, main) {
  // Simple test of removing inside the first chunk.
  {
    VariableWindowDataSet d;

    EXPECT_EQ(d.NumTransactions(), 0);
    d.Append(Transaction(0, "a,b,e"));
    EXPECT_EQ(d.NumTransactions(), 1);
    d.Append(Transaction(1, "a,c,e"));
    EXPECT_EQ(d.NumTransactions(), 2);
    d.Append(Transaction(2, "b,c,d,e"));
    EXPECT_EQ(d.NumTransactions(), 3);
    d.Append(Transaction(3, "c,d,e,f"));
    EXPECT_EQ(d.NumTransactions(), 4);
    d.Append(Transaction(4, "e"));
    EXPECT_EQ(d.NumTransactions(), 5);

    // Ensure dataset is correctly stored.
    EXPECT_EQ(d.Count(Item("a")), 2);
    EXPECT_EQ(d.Count(Item("b")), 2);
    EXPECT_EQ(d.Count(Item("c")), 3);
    EXPECT_EQ(d.Count(Item("d")), 2);
    EXPECT_EQ(d.Count(Item("e")), 5);
    EXPECT_EQ(d.Count(Item("f")), 1);
    EXPECT_EQ(d.Count(Item("g")), 0); // Invalid item.

    EXPECT_EQ(d.Count(ItemSet("a", "b")), 1);
    EXPECT_EQ(d.Count(ItemSet("a", "e")), 2);
    EXPECT_EQ(d.Count(ItemSet("c", "e")), 3);
    EXPECT_EQ(d.Count(ItemSet("g", "h")), 0); // Invalid itemset.

    //d.RemoveUpTo(0);
    d.Pop();
    EXPECT_EQ(d.NumTransactions(), 4);
    EXPECT_EQ(d.Count(Item("a")), 1);
    EXPECT_EQ(d.Count(Item("b")), 1);
    EXPECT_EQ(d.Count(Item("c")), 3);
    EXPECT_EQ(d.Count(Item("d")), 2);
    EXPECT_EQ(d.Count(Item("e")), 4);
    EXPECT_EQ(d.Count(Item("f")), 1);

    //d.RemoveUpTo(2);
    d.Pop();
    d.Pop();
    EXPECT_EQ(d.Count(Item("a")), 0);
    EXPECT_EQ(d.Count(Item("b")), 0);
    EXPECT_EQ(d.Count(Item("c")), 1);
    EXPECT_EQ(d.Count(Item("d")), 1);
    EXPECT_EQ(d.Count(Item("e")), 2);
    EXPECT_EQ(d.Count(Item("f")), 1);
    EXPECT_EQ(d.NumTransactions(), 2);

    //d.RemoveUpTo(3);
    d.Pop();
    EXPECT_EQ(d.Count(Item("a")), 0);
    EXPECT_EQ(d.Count(Item("b")), 0);
    EXPECT_EQ(d.Count(Item("c")), 0);
    EXPECT_EQ(d.Count(Item("d")), 0);
    EXPECT_EQ(d.Count(Item("e")), 1);
    EXPECT_EQ(d.Count(Item("f")), 0);
    EXPECT_EQ(d.NumTransactions(), 1);
  }
  {
    EXPECT_EQ(TestExpectedItemA(0, 0), 0);
    EXPECT_EQ(TestExpectedItemB(0, 0), 0);
    EXPECT_EQ(TestExpectedItemB(0, 1), 1);
    EXPECT_EQ(TestExpectedItemB(0, 2), 1);
    EXPECT_EQ(TestExpectedItemB(0, 3), 2);

    EXPECT_EQ(TestExpectedItemA(1, 3), 3);
    EXPECT_EQ(TestExpectedItemB(0, 4), 2);

    VariableWindowDataSet d;
    const uint32_t n = VariableWindowDataSet::chunk_size * 4 + 1;
    for (uint32_t i = 0; i < n; i++) {
      d.Append(MakeTestTransaction(i));
      EXPECT_EQ(d.Count(Item("a")), TestExpectedItemA(0, i + 1));
      EXPECT_EQ(d.Count(Item("b")), TestExpectedItemB(0, i + 1));
    }

    // Remove all the transactions, verify that the correct number of
    // items remain.
    for (uint32_t i = 0; i < n; i++) {
      Transaction t = d.Front();
      d.Pop();
      EXPECT_EQ(d.Count(Item("a")), TestExpectedItemA(i + 1, d.NumTransactions()));
      EXPECT_EQ(d.Count(Item("a")), d.NumTransactions());
      EXPECT_EQ(d.Count(Item("b")), TestExpectedItemB(i + 1, d.NumTransactions()));
      EXPECT_EQ(d.Count(ItemSet("a", "b")), TestExpectedItemB(i + 1, d.NumTransactions()));
    }
  }
  #ifdef INDEX_STRESS_TEST
  {
    cout << "Stress testing VariableWindowDataSet by loading kosarak..." << endl;
    VariableWindowDataSet d;
    DataSetReader reader;
    bool ok = reader.Open("datasets/kosarak.csv");
    EXPECT_EQ(ok);
    if (!ok) {
      cerr << "Failed to open kosarak.csv" << endl;
      return;
    }
    TransactionId tid = 0;
    while (true) {
      Transaction transaction(tid);
      if (!reader.GetNext(transaction.items)) {
        cout << "Finished, loaded " << tid << " transactions." << endl;
        break;
      }
      tid++;
      d.Append(transaction);
      if ((tid % 50000), 0) {
        cout << "Loaded up to transaction " << tid << endl;
      }
    }
  }
  #endif
}
