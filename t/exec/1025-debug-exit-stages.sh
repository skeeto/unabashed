# A DEBUG action that exits a command substitution after a pipeline's
# first stage started, before the last: the started stage runs on, its
# output still the substitution's, which waits for it, and none of it
# reaches a later substitution
for i in 1 2; do
  x=$(if [ $i = 1 ]; then exec 2>&1; trap 'exit 0' DEBUG; { sleep 0.3; echo late >&2; } | cat; else sleep 1; echo hi; fi)
  echo "x=[$x]"
done
# a program as the started stage
for i in 1 2; do
  x=$(if [ $i = 1 ]; then exec 2>&1; trap '[[ $BASH_COMMAND == cat* ]] && exit 4' DEBUG; ls /nonexistent | cat; else sleep 1; echo hi; fi)
  echo "$? x=[$x]"
done
# the action of a later stage, after two have started
x=$(exec 2>&1; trap '[[ $BASH_COMMAND == "cat -n" ]] && exit 5' DEBUG; { sleep 0.3; echo late2 >&2; } | { sleep 0.5; echo late3 >&2; } | cat -n)
echo "$? x=[$x]"
