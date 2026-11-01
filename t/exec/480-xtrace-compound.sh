set -x
for i in a 'b c'; do :; done
for j; do :; done
case x in x) : matched;; esac
case "a b" in a*) : ;; esac
if true; then : yes; elif false; then :; fi
while false; do :; done
until true; do :; done
{ : brace; }
! false
true && : and
false || : or
i=0; while (( i < 1 )); do (( i++ )); done
