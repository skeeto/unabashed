# && and || chains are left-associative with equal precedence
true && echo a || echo b
false && echo c || echo d
true || echo e && echo f
false || echo g && echo h
false && echo i && echo j || echo k
true || false || echo l; echo "status $?"
false || false || echo m
false && echo n; echo "and fails: $?"
true || echo o; echo "or skips: $?"
(exit 3) || (exit 4); echo "or last: $?"
(exit 3) && (exit 4); echo "and first: $?"
(exit 3) || true && (exit 5); echo "mix: $?"
false || (exit 6) || echo "p $?"
true &&
    echo "newline after &&"
false ||
    echo "newline after ||"
true && ! false && echo "negated middle"
! true || echo "negated first $?"
