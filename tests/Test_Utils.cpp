#include "gtest/gtest.h"

#include <vector>
#include "List.h"
#include "PatternStream.h"

using namespace std;

TEST(List, main) {
  List<int> list;
  for (int i = 0; i <= 20; i++) {
    list.Append(i);
  }
  EXPECT_EQ(list.GetSize(), 21);

  {
    vector<int> L;
    List<int>::Iterator* itr = list.Begin();
    while (itr->HasNext()) {
      L.push_back(itr->Next());
    }
    for (int i = 0; i <= 20; i++) {
      EXPECT_EQ(L[i], i);
    }
    delete itr;
  }

  List<int>::Iterator* itr = list.Begin();
  int expected = 0;
  while (itr->HasNext()) {
    List<int>::Token token = itr->GetToken();
    EXPECT_TRUE(token.IsInList());
    int x = itr->Next();
    if (expected % 2 == 0) {
      list.Erase(token);
      EXPECT_FALSE(token.IsInList());
    }
    EXPECT_EQ(x, expected);
    expected++;
  }
  EXPECT_EQ(expected, 21);
  EXPECT_EQ(list.GetSize(), 10);
  EXPECT_FALSE(itr->HasNext());
  for (int i = 21; i <= 40; i++) {
    list.Append(i);
  }
  EXPECT_TRUE(itr->HasNext());
  while (itr->HasNext()) {
    int x = itr->Next();
    EXPECT_EQ(x, expected);
    expected++;
  }
  EXPECT_FALSE(itr->HasNext());

  list.Prepend(-1);
  EXPECT_FALSE(itr->HasNext());
  itr = list.Begin();
  EXPECT_EQ(itr->Next(), -1);
  delete itr;
}


TEST(PatternStream, main) {
  cout << "Testing PatternOutputStream\n";
  time_t startTime = time(0);

  shared_ptr<std::ostringstream> sstream(make_shared<std::ostringstream>());
  PatternOutputStream out(sstream, nullptr);

  ItemSet itemset("a", "b", "c", "d");
  const unsigned num = 10000;
  for (unsigned i = 0; i < num; i++) {
    out.Write(itemset);
  }
  out.Close();
  time_t duration = time(0) - startTime;
  cout << "Finished in " << duration << " seconds\n";

  PatternInputStream in(make_shared<istringstream>(sstream->str()));
  EXPECT_TRUE(in.IsOpen());

  ItemSet x;
  unsigned count = 0;
  while (!(x = in.Read()).IsNull()) {
    EXPECT_EQ(x, itemset);
    count++;
  }
  EXPECT_EQ(count, num);
}
