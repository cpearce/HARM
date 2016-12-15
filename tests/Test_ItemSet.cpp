#include "gtest/gtest.h"
#include "ItemSet.h"
#include "ItemMap.h"

#include <iostream>
#include <string>

using namespace std;

TEST(Item, main) {

  Item dog("dog");
  Item cat("cat");

  EXPECT_NE(dog, cat);

  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  EXPECT_LT(cat, dog);

  Item::SetCompareMode(Item::INSERTION_ORDER_COMPARE);
  EXPECT_LT(dog, cat);

  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);
  
  int dogId = dog;
  int catId = cat;
  EXPECT_NE(dogId, catId);

  Item dog2("dog");
  EXPECT_EQ(dog2, dog);

  int dog2Id = dog2;
  EXPECT_EQ(dog2Id, dogId);

  string dogStr = dog;
  EXPECT_EQ(dogStr, "dog");
  string catStr = cat;
  EXPECT_EQ(catStr, "cat");

  dog = cat;
  EXPECT_EQ(dog, cat);
}

TEST(ItemSet, main) {
  Item::SetCompareMode(Item::ALPHABETIC_COMPARE);

  cout << __FUNCTION__ << "()\n";

  Item dog("dog");
  Item cat("cat");

  ASSERT_TRUE(cat < dog);
  ASSERT_TRUE(!(dog < cat));

  ItemSet a;
  a += dog;
  a += cat;

  string str = a;
  ASSERT_TRUE(str == "cat dog");

  ItemSet both("dog", "cat");
  ASSERT_TRUE(both == a);

  ItemSet bothClone(&both);
  ASSERT_TRUE(both == bothClone);

  ItemSet dogSet = dog;
  ASSERT_TRUE(dogSet < a);

  ItemSet b;
  b += Item("budgie");
  b += Item("sheep");
  b += a;
  str = b;
  ASSERT_TRUE(str == "budgie cat dog sheep");
  const char* items[] = {"sheep", "cat", "budgie", "dog"};
  ASSERT_TRUE(ItemSet(items, 4) == b);

  ItemSet c("budgie", "sheep");
  ItemSet ac(a, c);
  str = ac;
  ASSERT_TRUE(str == "budgie cat dog sheep");

  ItemSet d("budgie", "cat", "dog", "sheep");
  ItemSet e("budgie", "lion", "dog", "sheep");
  ASSERT_TRUE(d < e);
  ItemSet ed(e, d);
  str = ed;
  ASSERT_TRUE(str == "budgie cat dog lion sheep");
  ItemSet de(d, e);
  str = de;
  ASSERT_TRUE(str == "budgie cat dog lion sheep");
  ASSERT_TRUE(de == ed);

  Item sheep("sheep");
  ItemSet k = de - sheep;
  // Make sure the "-" operator didn't change the original.
  str = de;
  ASSERT_TRUE(str == "budgie cat dog lion sheep");
  // Ensure the "-" worked...
  str = k;
  ASSERT_TRUE(str == "budgie cat dog lion");

}

TEST(ItemMap, main) {
  std::map<int, unsigned> expected;
  ItemMap<unsigned> m;
  for (unsigned i = 1; i < 1000; ++i) {
    Item item(i);
    m.Set(item, i);
    expected[item.GetId()] = i;
  }

  ItemMap<unsigned>::Iterator itr = m.GetIterator();
  int count = 0;
  while (itr.HasNext()) {
    Item item = itr.GetKey();
    unsigned value = itr.GetValue();
    EXPECT_EQ(value, expected[item.GetId()]);
    itr.Next();
    count++;
  }
  EXPECT_EQ(count, expected.size());
}