set -u
f() { echo "in f"; echo "$undefined_in_f"; echo "after in f"; }
f
echo notreached
