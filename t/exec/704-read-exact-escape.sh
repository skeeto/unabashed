# read -N without -r still takes backslash escapes, from text in memory
# as from a pipe or file
read -N 3 v <<< 'a\bcd'; echo "1 $? [$v]"
printf 'a\bcd' | { read -N 3 v; echo "2 $? [$v]"; }
read -N 4 v <<< 'a\'; echo "3 $? [$v]"
read -rN 3 v <<< 'a\bcd'; echo "4 $? [$v]"
