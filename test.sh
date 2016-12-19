
# Remove the old output, so our comparisons aren't messed up by previous results
# if we fail.
mkdir test-output
rm test-output/*


tests=( fp-test3 fp-test4 fp-test5 fp-test fp-test2 test single-path census1 UCI-adult )
for i in "${tests[@]}"
do
	echo $i

  build/Release/Harm.exe -i datasets/test/$i.csv -m apriori -o test-output/apriori-$i -minsup 0.2
  echo ==================================================================
  build/Release/Harm.exe -i datasets/test/$i.csv -m fptree -o test-output/fptree-$i -minsup 0.2
  echo ==================================================================
  build/Release/Harm.exe -i datasets/test/$i.csv -m cantree -o test-output/cantree-$i -minsup 0.2
  echo ========================================	==========================
  build/Release/Harm.exe -i datasets/test/$i.csv -m cptree -o test-output/cptree-$i -minsup 0.2 -cp-sort-interval 1000
  echo ==================================================================
  build/Release/Harm.exe -i datasets/test/$i.csv -m spotree -o test-output/spotree-$i -minsup 0.2 -spo-entropy-threshold 0.1

  sort < test-output/fptree-$i.itemsets.support.csv > test-output/fptree-$i.itemsets.support.sorted.csv
  sort < test-output/cantree-$i.itemsets.support.csv > test-output/cantree-$i.itemsets.support.sorted.csv
  sort < test-output/cptree-$i.itemsets.support.csv > test-output/cptree-$i.itemsets.support.sorted.csv
  sort < test-output/apriori-$i.itemsets.support.csv > test-output/apriori-$i.itemsets.support.sorted.csv
  sort < test-output/spotree-$i.itemsets.support.csv > test-output/spotree-$i.itemsets.support.sorted.csv

  diff -U4 test-output/apriori-$i.itemsets.support.sorted.csv test-output/fptree-$i.itemsets.support.sorted.csv
  RES=$?
  echo "$RES"
  if [ "$RES" = 0 ]; then
    echo PASSED: FPTREE $i
  else
    echo FAILED: FPTREE $i
    exit -1
  fi
  
  diff -U4 test-output/apriori-$i.itemsets.support.sorted.csv test-output/cantree-$i.itemsets.support.sorted.csv
  RES=$?
  echo "$RES"
  if [ "$RES" = 0 ]; then
    echo PASSED CANTREE $i
  else
    echo FAILED CANTREE $i
    exit -1
  fi

  diff -U4 test-output/apriori-$i.itemsets.support.sorted.csv test-output/cptree-$i.itemsets.support.sorted.csv
  RES=$?
  echo "$RES"
  if [ "$RES" = 0 ]; then
    echo PASSED CPTREE $i
  else
    echo FAILED CPTREE $i
    exit -1
  fi

  diff -U4 test-output/apriori-$i.itemsets.support.sorted.csv test-output/spotree-$i.itemsets.support.sorted.csv
  RES=$?
  echo "$RES"
  if [ "$RES" = 0 ]; then
    echo PASSED spotree $i
  else
    echo FAILED spotree $i
    exit -1
  fi
  
done


# Test that streaming UCI-zoo with block size of 25 is the same as mining
# the UCI-zoo dataset partitioned into 25 line chunks.

# First mine the dataset in streaming mode.
build/Release/Harm.exe -i datasets/test/UCI-zoo.csv -m stream -o test-output/stream-UCI-zoo -minsup 0.6 -blockSize 25

# Mine each quarter of UCI zoo using regular apriori.
for i in {1..4}
do
  build/Release/Harm.exe -i datasets/test/UCI-zoo-$i.csv -m apriori -o test-output/apriori-UCI-zoo-$i -minsup 0.6
done

# Compare each output chunk.
for i in {1..4}
do
  # sort output so we can compare cleanly.
  sort < test-output/apriori-UCI-zoo-$i.itemsets.support.csv > test-output/apriori-UCI-zoo-$i.itemsets.support.sorted.csv
  sort < test-output/stream-UCI-zoo.itemsets.support-$i.csv > test-output/stream-UCI-zoo.itemsets.support-$i.sorted.csv
 
  diff -U4 test-output/apriori-UCI-zoo-$i.itemsets.support.sorted.csv test-output/stream-UCI-zoo.itemsets.support-$i.sorted.csv
  RES=$?

  if [ "$RES" = 0 ]; then
    echo PASSED: streaming UCI-zoo-${i}
  else
    echo FAILED: streaming UCI-zoo-${i}
    echo test-output/apriori-${i}.itemsets.support.sorted.csv test-output/stream-${i}.itemsets.support.sorted.csv
    exit -1
  fi

done

echo
echo All tests passed successfully!