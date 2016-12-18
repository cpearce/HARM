#include "gtest/gtest.h"
#include "FPTree.h"
#include "FPNode.h"
#include "TestDataSets.h"

#include <string>
#include <iostream>

using namespace std;

extern FPTree* CreateFPTree(DataSet* aDataSet, Options& options);

extern void FPGrowth(FPTree* tree,
                     PatternOutputStream& output,
                     vector<Item>& pattern,
                     DataSet* index,
                     const double minCount,
                     unsigned nodePruneDepth = std::numeric_limits<unsigned>::max(),
                     ItemFilter* = nullptr);

extern void AddPatternsInPath(const FPNode* tree,
                              PatternOutputStream& output,
                              vector<Item>& pattern,
                              unsigned maxNodeDepth = UINT32_MAX,
                              ItemFilter* filter = nullptr);

extern void ConstructConditionalTree(const FPNode* node,
                              FPTree* tree,
                              double minCount,
                              unsigned nodePruneDepth = std::numeric_limits<unsigned>::max());

static string GetPath(FPNode* n) {
  string path;
  while (!n->IsRoot()) {
    string s = n->item;
    char buf[32];
    sprintf(buf, "%s:%u%s", s.c_str(), n->count, (n->parent->IsRoot() ? "" : " "));
    path.append(buf);
    n = n->parent;
  }
  return path;
}

/*
i1,i2,i5
i2,i4
i2,i3
i1,i2,i4
i1,i3
i2,i3
i1,i3
i1,i2,i3,i5
i1,i2,i3
*/
TEST(FPTree, TestInitialConstruction) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  InvertedDataSetIndex index(FPTestDataSet());
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  options.inputFileName = "datasets/test/fp-test.csv";
  FPTree* fptree = CreateFPTree(&index, options);
  EXPECT_TRUE(!!fptree);
  index.Load();

  string s = fptree->ToString();
  EXPECT_EQ(s, "(root:0 (i1:2 (i3:2)) (i2:7 (i1:4 (i3:2 (i5:1)) (i4:1) (i5:1)) (i3:2) (i4:1)))");
  EXPECT_EQ(fptree->NumNodes(), 11);

  shared_ptr<std::ostringstream> stream(make_shared<std::ostringstream>());
  PatternOutputStream output(stream, 0);
  vector<Item> pattern;
  FPGrowth(fptree, output, pattern, &index, 0);
  output.Close();

  ItemSet expected[] = {
    {"i1"},
    {"i1","i2"},
    {"i2"},
    {"i5"},
    {"i2","i5"},
    {"i1","i5"},
    {"i3","i5"},
    {"i1","i3","i5"},
    {"i1","i2","i5"},
    {"i2","i3","i5"},
    {"i1","i2","i3","i5"},
    {"i4"},
    {"i2","i4"},
    {"i1","i4"},
    {"i1","i2","i4"},
    {"i3"},
    {"i1","i3"},
    {"i1","i2","i3"},
    {"i2","i3"},
  };

  PatternInputStream input(make_shared<istringstream>(stream->str()));
  EXPECT_TRUE(input.IsOpen());
  ItemSet itemset;
  size_t expectedIndex = 0;
  while (!(itemset = input.Read()).IsNull()) {
    EXPECT_EQ(string(itemset), string(expected[expectedIndex++]));
  }
  EXPECT_FALSE(fptree->HasSinglePath());

  // Test the headerTable was maintained during insertion.
  // Check that i5 has two paths, and that they're correct.
  /*FPNode* n = fptree->headerTable->find(Item("i5"))->second;*/
  FPNode* n = fptree->HeaderTable().Get(Item("i5"));

  string path = GetPath(n);
  EXPECT_EQ(path, "i5:1 i3:2 i1:4 i2:7");

  n = n->next;
  EXPECT_TRUE(n && !n->next); // Should have one more path for i5.
  path = GetPath(n);
  EXPECT_EQ(path, "i5:1 i1:4 i2:7");

  delete fptree;
}

TEST(FPTree, HasSinglePath) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  InvertedDataSetIndex index(SinglePathDataSetReader());
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  options.inputFileName = "datasets/test/single-path.csv";
  FPTree* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string s = fptree->ToString();
  EXPECT_EQ(s, "(root:0 (i1:2 (i2:2 (i3:2))))");
  EXPECT_EQ(fptree->NumNodes(), 4);

  EXPECT_TRUE(fptree->HasSinglePath());
}

template<class T>
string Flatten(const vector<T>& v) {
  string s("(");
  for (unsigned i = 0; i < v.size(); i++) {
    string item = v[i];
    s.append(item);
    if (i + 1 != v.size()) {
      s.append(",");
    }
  }
  s.append(")");
  return s;
}

TEST(FPTree, AddPatternsInPath) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  FPTree* t = new FPTree();
  vector<Item> itemset;
  itemset.push_back(Item("i1"));
  itemset.push_back(Item("i2"));
  itemset.push_back(Item("i3"));
  itemset.push_back(Item("i4"));
  t->Insert(itemset);

  string s = t->GetRoot()->ToString();
  cout << s.c_str() << endl;

  shared_ptr<std::ostringstream> sstream(make_shared<std::ostringstream>());
  PatternOutputStream output(sstream, nullptr);
  vector<Item> path;
  AddPatternsInPath(t->GetRoot()->FirstChild(), output, path);
  output.Close();

  PatternInputStream input(make_shared<istringstream>(sstream->str()));
  EXPECT_TRUE(input.IsOpen());
  vector<ItemSet> v = input.ToVector();
  cout << "Results:\n";
  s = Flatten(v);
  EXPECT_EQ(s, "(i1,i2,i3,i4,i3 i4,i2 i3,i2 i4,i2 i3 i4,i1 i2,i1 i3,i1 i4,i1 i3 i4,i1 i2 i3,i1 i2 i4,i1 i2 i3 i4)");

}

void TestConstructConditionalTree_inner(DataSet* index,
                                        FPTree* condTree,
                                        const char* item,
                                        const char* res_conf0,
                                        const unsigned min_count,
                                        const char* res_conf_count) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  FPNode* headerList = condTree->HeaderTable().Get(Item(item));
  EXPECT_TRUE(!!headerList);

  {
    FPTree* tree = new FPTree();
    ConstructConditionalTree(headerList, tree, 0);
    string s = tree->ToString();
    EXPECT_EQ(s, res_conf0);
    delete tree;
  }
  {
    FPTree* tree = new FPTree();
    ConstructConditionalTree(headerList, tree, min_count);
    string s = tree->ToString();
    EXPECT_EQ(s, res_conf_count);
    delete tree;
  }
}

TEST(FPTree, ConstructConditionalTree) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  {
    InvertedDataSetIndex index(FPTestDataSet());
    Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
    options.inputFileName = "datasets/test/fp-test.csv";
    FPTree* fptree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!fptree);
    index.Load();

    cout << "Tree: " << fptree->ToString() << endl;

    TestConstructConditionalTree_inner(&index, fptree, "i5", "(root:0 (i2:2 (i1:2 (i3:1))))", 2, "(root:0 (i2:2 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i4", "(root:0 (i2:2 (i1:1)))", 2, "(root:0 (i2:2))");
    TestConstructConditionalTree_inner(&index, fptree, "i3", "(root:0 (i1:2) (i2:4 (i1:2)))", 3, "(root:0 (i1:2) (i2:4 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i3", "(root:0 (i1:2) (i2:4 (i1:2)))", 2, "(root:0 (i1:2) (i2:4 (i1:2)))");
    TestConstructConditionalTree_inner(&index, fptree, "i2", "(root:0)", 2, "(root:0)");
    TestConstructConditionalTree_inner(&index, fptree, "i1", "(root:0 (i2:4))", 5, "(root:0)");

    delete fptree;
  }
  {
    InvertedDataSetIndex index(FPTest5DataSetReader());
    Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
    options.inputFileName = "datasets/test/fp-test5.csv";
    FPTree* fptree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!fptree);
    index.Load();

    cout << "Tree: " << fptree->ToString() << endl;
    TestConstructConditionalTree_inner(&index, fptree, "f=1", "(root:0 (e=1:2 (b=0:1)))", 2, "(root:0 (e=1:2))");
  }
}

#if 0
// TODO: Make these tests EXPECT something.
static void TestFPGrowth() {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  InvertedDataSetIndex index(FPTestDataSet());
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  options.inputFileName = "datasets/test/fp-test.csv";
  FPTree* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string ts = fptree->ToString();
  cout << "Tree: " << ts << endl;

  PatternOutputStream output("datasets/test/fp-test-itemsets.csv", 0);
  vector<Item> vec;
  FPGrowth(fptree, output, vec, &index, 2);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/fp-test-itemsets.csv");
  vector<ItemSet> v = input.ToVector();

  cout << "TestFPGrowth() generated rules (" << v.size() << "):" << endl;
  for (unsigned i = 0; i < v.size(); ++i) {
    cout << ((string)v[i]).c_str() << endl;
  }
  cout << Flatten(v).c_str() << endl;

  delete fptree;
}

static void TestFPGrowth2() {
  cout << endl << "test fp3" << endl;
  // -i datasets/test/fp-test3.csv -m fptree -o output/fptree-fp-test3 -minsup 0.2
  InvertedDataSetIndex index(FPTest3DataSetReader());
  Options options(0, kFPTree, 0, 0, 0, 0, 0, 0, 0);
  options.inputFileName = "datasets/test/fp-test3.csv";
  FPTree* fptree = CreateFPTree(&index, options);
  if (!fptree) {
    return;
  }
  index.Load();

  string ts = fptree->ToString();
  cout << "Tree: " << ts << endl;

  PatternOutputStream output("datasets/test/fptree-fp-test-3-itemsets.csv", 0);
  double minCount = index.NumTransactions() * 0.2;
  vector<Item> vec;
  FPGrowth(fptree, output, vec, &index, minCount);
  output.Close();

  PatternInputStream input;
  input.Open("datasets/test/fptree-fp-test-3-itemsets.csv");
  vector<ItemSet> v = input.ToVector();

  cout << "TestFPGrowth() generated rules (" << v.size() << "):" << endl;
  for (unsigned i = 0; i < v.size(); ++i) {
    cout << ((string)v[i]).c_str() << endl;
  }
  cout << Flatten(v).c_str() << endl;

  delete fptree;
}
#endif

TEST(FPTree, Stream) {
  {
    Options options(0, kStream, UINT_MAX, 0, 0, 0, 0, 0, 5);
    options.inputFileName = "datasets/test/census2.csv";
    options.outputFilePrefix = "census2-out";

    InvertedDataSetIndex index(Census2DataSetReader());
    FPTree* cantree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!cantree);
    if (!cantree) {
      return;
    }
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    //cantree->DumpToGraphViz("test-output/cptree-census2.dot");

    printf("freq before sort:\n");
    cantree->DumpFreq();
    cantree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    cantree->DumpFreq();

    //cantree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    EXPECT_TRUE(cantree->IsSorted());

    delete cantree;
  }
}

TEST(FPTree, TreeSorted) {
  {
    InvertedDataSetIndex index(Census2DataSetReader());
    Options options(0, kFPTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    options.inputFileName = "datasets/test/census2.csv";
    FPTree* cantree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!cantree);
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    //cantree->DumpToGraphViz("datasets/test/census.dot");

    cantree->Sort();
    EXPECT_TRUE(cantree->IsSorted());

    delete cantree;
  }

  {
    vector<Item> v;
    for (unsigned i = 0; i < 4; i++) {
      char buf[4];
      sprintf(buf, "%c", (char)('a' + i));
      v.push_back(Item(buf));
    }
    string s = Flatten(v);
    ASSERT(s == "(a,b,c,d)");
    std::reverse(v.begin(), v.end());
    s = Flatten(v);
    EXPECT_EQ(s, "(d,c,b,a)");
  }

  {
    Item::ResetBaseId();

    InvertedDataSetIndex index(FPTest3DataSetReader());
    Options options(0, kCpTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    FPTree* cantree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!cantree);
    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    //cantree->DumpToGraphViz("test-output/cp-tree-fp-test3.dot");

    EXPECT_TRUE(cantree->IsSorted());

    cantree->Sort();
    //cantree->DumpToGraphViz("test-output/cp-tree-fp-test3.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    EXPECT_TRUE(cantree->IsSorted());

    delete cantree;
  }
  {
    Item::ResetBaseId();

    InvertedDataSetIndex index(Census2DataSetReader());
    Options options(0, kCpTree, UINT_MAX, 0, 0, 0, 0, 0, 0);
    FPTree* cantree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!cantree);

    index.Load();

    string ts = cantree->ToString();
    cout << "Tree: " << ts << endl;
    //cantree->DumpToGraphViz("test-output/cptree-census2.dot");

    printf("freq before sort:\n");
    cantree->DumpFreq();
    cantree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    cantree->DumpFreq();

    //cantree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = cantree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    EXPECT_TRUE(cantree->IsSorted());

    delete cantree;
  }
}

TEST(FPTree, SpoTree) {
  {
    ItemMap<unsigned> m;
    for (unsigned i = 1; i < 100; i++) {
      Item item(i);
      EXPECT_FALSE(m.Contains(item));
      m.Set(item, i);
      EXPECT_TRUE(m.Contains(item));
      EXPECT_TRUE(m.Get(item) == i);
    }
  }
  {
    Item::ResetBaseId();

    InvertedDataSetIndex index(Census2DataSetReader());
    Options options(0, kSpoTree, 0, 0, 0.15, 0, 0, 0, 0);
    FPTree* spotree = CreateFPTree(&index, options);
    EXPECT_TRUE(!!spotree);
    index.Load();

    string ts = spotree ->ToString();
    cout << "Tree: " << ts << endl;
    //spotree->DumpToGraphViz("test-output/spotree-census2.dot");

    EXPECT_TRUE(spotree->IsSorted());
    /*
    printf("freq before sort:\n");
    spotree->DumpFreq();
    spotree->Sort();

    printf("\n\nfreq AFTER sort:\n");
    spotree->DumpFreq();

    //spotree->DumpToGraphViz("test-output/cptree-census2.sorted.dot");
    ts = spotree->ToString();
    cout << "Sorted Tree: " << ts << endl;
    ASSERT(spotree->IsSorted());*/

    delete spotree;
  }
}
