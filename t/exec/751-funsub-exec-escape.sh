# A descriptor exec kept in ${ } outlives the substitution however it
# ends, and writing plenty there afterwards stays harmless
x="${ exec 4>&1; echo in; }"
for i in {1..3000}; do echo written-after-the-substitution-$i >&4; done
echo "st=$? x=[$x]"
let "1 =="
exec 4>&-
( trap 'for i in {1..3000}; do echo from-the-trap-$i >&5; done
         echo "trap st=$?"; let "1 =="; echo done' EXIT
  x="${ exec 5>&1; echo in; exit 3; }"
  echo not reached )
echo "exit st=$?"
( set -u
  trap 'for i in {1..3000}; do echo from-the-trap-$i >&6; done
        echo "trap st=$?"' EXIT
  x="${ exec 6>&1; echo "$unset"; }"
  echo not reached )
echo "unset st=$?"
trap 'for i in {1..3000}; do echo at-exit-$i >&7; done; echo "top st=$?"' EXIT
x="${ exec 7>&1; exit 4; }"
