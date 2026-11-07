# set -o with no option name after it lists, and parses on
exec 2>&1
set -o -B | head -3; echo st=$?
set -o -B 2>&1 | wc -l
set +o -x | head -2; set +x
set -o "" | wc -l
set -o -- a b | wc -l; echo "$@"
