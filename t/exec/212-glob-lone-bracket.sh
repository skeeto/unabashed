# A [ without a closing ] does not make a word a pattern, so nullglob
# and failglob leave it alone.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > ab
shopt -s nullglob
show [ a[ [ab a[b x]
show [] []] [!]
show [*
show [a]*
shopt -u nullglob
shopt -s failglob
show [ a[ [ab
show [z]
echo "status $?"
show [z
echo end
