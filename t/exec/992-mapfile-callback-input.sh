# A mapfile callback may replace the input it reads, freeing the here-
# document: the rest of the lines come from what the descriptor is now
secret=x
cb() { exec 0</dev/null; secret=SECRET-SECRET-SECRET-SECRET-SECRET-SECRET; }
mapfile -t -C cb -c 1 arr <<EOT
line one of the here-document, long enough to share a class
line two of the here-document, long enough to share a class
line three
EOT
declare -p arr
cb3() { read -r skipped; }
mapfile -t -C cb3 -c 2 arr3 <<EOT
a
b
c
d
e
EOT
declare -p arr3 skipped
f() { mapfile -t -C 'exit 3 #' -c 1 arr4 <<< $'x\ny'; }
( f ); echo "status $?"
