split() { local IFS=,; set -- $1; echo "$# fields: $*"; }
split "a,b,c"
v="x y z"
set -- $v
echo "top: $#"
split "1,2"
set -- $v
echo "top again: $#"
joiner() { local IFS=-; echo "$*"; }
joiner p q r
echo "IFS length ${#IFS}"
