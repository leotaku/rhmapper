# rhmapper.h
> Special-purpose robin hood hashmap implementation for C99

+ Maps strings/arbitrary data to monotonically increasing integers
+ Does not support deletions and ignores reassignment
+ Optimized for large amounts of access operations to few keys
+ Reverse lookup behind RHMAPPER_REVERSE preprocessor option

It is extremely likely that this implementation is not properly optimized. Either the average and maximum probe counts shown [here](https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/) are off, or my implementation is wrong.
