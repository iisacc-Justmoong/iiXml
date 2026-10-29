# Serialization costs

The C++23 serializer sorts fragments by start ascending and end descending, then
tracks the furthest emitted end. Once starts are ordered, this end alone decides
whether an earlier emitted range contains the candidate. It avoids scanning and
copying all earlier fragments for every candidate. Sorting remains O(n log n),
followed by an O(n) containment pass, including overlapping, nested and empty
ranges. Partial overlaps still produce the same concatenated fragments.

Merged serialization already sorts and deduplicates source indexes; membership
now uses binary search in that list rather than a full linear scan per block.
Tests compare output with an independent all-prior-ranges oracle and cover
10,000 adjacent fragments, nested ranges and invalid source indexes.
