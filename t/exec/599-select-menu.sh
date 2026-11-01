exec 2>&1
COLUMNS=40
select x in $(seq 1 25); do :; done </dev/null; echo st=$?
select x in aaaaaaaaaaaa bbbbbbbbbbbbbbb c d e f g h i j k l; do :; done </dev/null
select x in a b c d e f g h i j k l m n o p; do :; done </dev/null
PS3="> "; select x in a b c; do echo "got $x $REPLY"; break; done <<< "2"
set -- p q; select x; do echo $x; break; done <<< 1
select x in a b; do echo "[$x] $REPLY"; done <<< "$(printf '\n5\n 2 \n')"
select x in; do echo never; done; echo empty=$?
