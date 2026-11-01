# DEBUG before case and select (BASH_COMMAND their headers), and for's
# own line on every iteration
trap 'echo "debug $LINENO [$BASH_COMMAND]"' DEBUG
case x$((1)) in
 x1) echo one;;
esac
for i in 1 2; do
  case $i in
  2) echo two;;
  esac
done
select s in a; do break; done </dev/null 2>/dev/null
while false; do :; done
if case y in y) true;; esac; then echo yes; fi
for i in 0 1; do
  for j in 3 4; do
    : $i $j
  done
done
