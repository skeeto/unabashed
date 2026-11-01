# if/elif/else chains pick exactly one branch
for n in 1 2 3 4; do
    if [ $n = 1 ]; then
        echo "$n: one"
    elif [ $n = 2 ]; then
        echo "$n: two"
    elif [ $n = 3 ]
    then echo "$n: three"
    else
        echo "$n: other"
    fi
done
if true; then echo a; fi
if false; then echo b; else echo c; fi
if false; then echo d; elif false; then echo e; fi
echo "no branch: $?"
if false; then :; elif true; then echo f; else echo g; fi
