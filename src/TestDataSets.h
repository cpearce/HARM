// Copyright 2016, Chris Pearce & Yun Sing Koh
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

#include <string>
#include <memory>
#include <sstream>
#include "DataSetReader.h"

std::unique_ptr<DataSetReader> Test1DataSetReader()
{
  // datasets/test/test1.data
  const std::string data =
    "a, b, c, d, e, f\n"
    "z, h, i, j, k, l\n"
    "z, x\n"
    "z, x\n"
    "z, x, y\n"
    "z, x, y, i\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> Test2DataSetReader()
{
  // Test partially dirty data, tests parsing/tokenizing.
  // datasets/test/test2.data
  const std::string data =
    "aa, bb, cc , dd\n"
    "aa, gg,\n"
    "aa, bb\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> Test3DataSet()
{
  // datasets/test/test3.data
  const std::string data =
    "a, b, c, d, e, f, o\n"
    "a, f, g, h, u, e, s\n"
    "c, y, f, d, k, s, a\n"
    "r, t, y, u, i, q, a\n"
    "c, g, s, o, t, a, u\n"
    "a, s, y, r, y, u, w\n"
    "a, b, c, f, r, d, e\n"
    "e, r, s, z, t, e, w\n"
    "q, w, r, k, t, y, u\n"
    "g, h, s, j, u, w, i\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> Test4DataSet()
{
  // datasets/test/test4.data
  const std::string data =
    "1, 3, 4\n"
    "2, 3, 5\n"
    "1, 2, 3, 5\n"
    "2, 5\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> FPTestDataSet()
{
  // datasets/test/fp-test.csv
  const std::string data =
    "i1, i2, i5\n"
    "i2, i4\n"
    "i2, i3\n"
    "i1, i2, i4\n"
    "i1, i3\n"
    "i2, i3\n"
    "i1, i3\n"
    "i1, i2, i3, i5\n"
    "i1, i2, i3\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> SinglePathDataSetReader()
{
  // datasets/test/single-path.csv
  const std::string data =
    "i1, i2, i3\n"
    "i1, i2, i3\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> FPTest5DataSetReader()
{
  // datasets/test/fp-test5.csv
  const std::string data =
    "b=1, e=1, f=1\n"
    "b=0, e=1, f=1\n"
    "b=0\n"
    "b=0\n"
    "b=0\n"
    "b=0\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> FPTest3DataSetReader()
{
  // datasets/test/fp-test3.csv
  const std::string data =
    "a\n"
    "b\n"
    "b, c, d\n"
    "a, c, d\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> Census2DataSetReader()
{
  // datasets/test/fp-test3.csv
  const std::string data =
    "a, b, c\n"
    "d, b, c\n"
    "a, b, e\n"
    "f, g, c\n"
    "d, g, e\n"
    "f, b, c\n"
    "f, b, c\n"
    "a, b, e\n"
    "a, b, c\n"
    "a, b, e\n"
    "a, b, e\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}

std::unique_ptr<DataSetReader> UCIZooDataSetReader()
{
  // datasets/test/UCI-zoo.csv
  const std::string data =
    "aardvark,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=0,domestic=0,catsize=1,type=1\n"
    "antelope,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "bass,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "bear,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=0,domestic=0,catsize=1,type=1\n"
    "boar,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "buffalo,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "calf,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=1,type=1\n"
    "carp,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=0,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=1,catsize=0,type=4\n"
    "catfish,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "cavy,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=0,domestic=1,catsize=0,type=1\n"
    "cheetah,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "chicken,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=1,catsize=0,type=2\n"
    "chub,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "clam,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=0,tail=0,domestic=0,catsize=0,type=7\n"
    "crab,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=4,tail=0,domestic=0,catsize=0,type=7\n"
    "crayfish,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=7\n"
    "crow,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "deer,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "dogfish,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=4\n"
    "dolphin,hair=0,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=1\n"
    "dove,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=1,catsize=0,type=2\n"
    "duck,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=1,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "elephant,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "flamingo,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "flea,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "frog,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=0,domestic=0,catsize=0,type=5\n"
    "frog,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=1,fins=0,legs=4,tail=0,domestic=0,catsize=0,type=5\n"
    "fruitbat,hair=1,feathers=0,eggs=0,milk=1,airbourne=1,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=1\n"
    "giraffe,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "girl,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=0,domestic=1,catsize=1,type=1\n"
    "gnat,hair=0,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "goat,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=1,type=1\n"
    "gorilla,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=0,domestic=0,catsize=1,type=1\n"
    "gull,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=1,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "haddock,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=0,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "hamster,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=0,type=1\n"
    "hare,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=1\n"
    "hawk,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "herring,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "honeybee,hair=1,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=1,fins=0,legs=6,tail=0,domestic=1,catsize=0,type=6\n"
    "housefly,hair=1,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "kiwi,hair=0,feathers=1,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "ladybird,hair=0,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=1,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "lark,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "leopard,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "lion,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "lobster,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=7\n"
    "lynx,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "mink,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "mole,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=1\n"
    "mongoose,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "moth,hair=1,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "newt,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=5\n"
    "octopus,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=8,tail=0,domestic=0,catsize=1,type=7\n"
    "opossum,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=1\n"
    "oryx,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "ostrich,hair=0,feathers=1,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "parakeet,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=1,catsize=0,type=2\n"
    "penguin,hair=0,feathers=1,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "pheasant,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "pike,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=4\n"
    "piranha,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "pitviper,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=1,fins=0,legs=0,tail=1,domestic=0,catsize=0,type=3\n"
    "platypus,hair=1,feathers=0,eggs=1,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "polecat,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "pony,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=1,type=1\n"
    "porpoise,hair=0,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=1\n"
    "puma,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "pussycat,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=1,type=1\n"
    "raccoon,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "reindeer,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=1,catsize=1,type=1\n"
    "rhea,hair=0,feathers=1,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "scorpion,hair=0,feathers=0,eggs=0,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=0,backbone=0,breathes=1,venomous=1,fins=0,legs=8,tail=1,domestic=0,catsize=0,type=7\n"
    "seahorse,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=0,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "seal,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=1,legs=0,tail=0,domestic=0,catsize=1,type=1\n"
    "sealion,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=1,legs=2,tail=1,domestic=0,catsize=1,type=1\n"
    "seasnake,hair=0,feathers=0,eggs=0,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=1,fins=0,legs=0,tail=1,domestic=0,catsize=0,type=3\n"
    "seawasp,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=1,fins=0,legs=0,tail=0,domestic=0,catsize=0,type=7\n"
    "skimmer,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=1,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "skua,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=1,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "slowworm,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=0,tail=1,domestic=0,catsize=0,type=3\n"
    "slug,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=0,tail=0,domestic=0,catsize=0,type=7\n"
    "sole,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=0,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=0,type=4\n"
    "sparrow,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n"
    "squirrel,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=1\n"
    "starfish,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=0,backbone=0,breathes=0,venomous=0,fins=0,legs=5,tail=0,domestic=0,catsize=0,type=7\n"
    "stingray,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=1,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=4\n"
    "swan,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=1,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "termite,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "toad,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=0,domestic=0,catsize=0,type=5\n"
    "tortoise,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=3\n"
    "tuatara,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=3\n"
    "tuna,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=1,predactor=1,toothed=1,backbone=1,breathes=0,venomous=0,fins=1,legs=0,tail=1,domestic=0,catsize=1,type=4\n"
    "vampire,hair=1,feathers=0,eggs=0,milk=1,airbourne=1,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=1\n"
    "vole,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=0,type=1\n"
    "vulture,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=1,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=2\n"
    "wallaby,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=0,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=1,type=1\n"
    "wasp,hair=1,feathers=0,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=1,fins=0,legs=6,tail=0,domestic=0,catsize=0,type=6\n"
    "wolf,hair=1,feathers=0,eggs=0,milk=1,airbourne=0,aquatic=0,predactor=1,toothed=1,backbone=1,breathes=1,venomous=0,fins=0,legs=4,tail=1,domestic=0,catsize=1,type=1\n"
    "worm,hair=0,feathers=0,eggs=1,milk=0,airbourne=0,aquatic=0,predactor=0,toothed=0,backbone=0,breathes=1,venomous=0,fins=0,legs=0,tail=0,domestic=0,catsize=0,type=7\n"
    "wren,hair=0,feathers=1,eggs=1,milk=0,airbourne=1,aquatic=0,predactor=0,toothed=0,backbone=1,breathes=1,venomous=0,fins=0,legs=2,tail=1,domestic=0,catsize=0,type=2\n";
  return std::make_unique<DataSetReader>(std::make_unique<std::istringstream>(data));
}
