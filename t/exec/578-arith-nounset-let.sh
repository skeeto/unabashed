set -u
x=1
let 'x += 1'; echo "x=$x"
let 'y = nope + 1'; echo "after let $?"
echo unreachable
