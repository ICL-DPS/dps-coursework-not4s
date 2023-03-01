* DPS-Coursework-2023

Discuss questions like “How does your implementation deal with the data characteristics?” and “How did you decide
tuning parameters like join order?”

We have first implemented sort-merge join then hash join. This was because we thought that hash join is more time consuming as it requires building a hash table for each input relation and the hashing each key, which are both computationally expensive. On the other hand, sort-merge only requires sorting the input relations, which can be done efficiently using external sorting algorithms even for very large datasets.

By sorting the input relations before performing the hash join, we can take advantage of the fact that the input data is already ordered and perform a more efficient join operation. Additionally, the size of the hash table can be minimized, since the input relations are already sorted and we can use a smaller hash table to build the join result.

However, we assume that our <e,f> is much more smaller than both <a,b> and <c,d>, we realised that it would be more time efficient to perform the smaller join first, as it would reduce the size of the larger table, <c,d>, which will make the subsequent sort-merge join more efficient. Hash join on <c,d> and <e,f> will create a new joined table which is smaller size than the original <c,d>. Therefore, there will be less unnecessary joins performed between the two large tables. Additionally, hash joins are typically faster than sort-merge joins, so you may be able to complete the smaller join more quickly and move on to the larger join.

Therefore, we changed our implementaiton to hash the e value of <e,f> table first, then do a hash join it with <c,d>. If there is a matching value, we go through the <a,b> table and find a matching value and save it in the results.

