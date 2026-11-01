# Here-documents as bash 5.3 gives them to commands: empty is /dev/null,
# up to 64 KiB a pipe filled first, larger a deleted temporary file.
# Programs and builtins share the one open file, so each reads on where
# the last stopped.
fits=$(printf '%065535d' 0)
big=$(printf '%065536d' 0)
[ -c /dev/stdin ] <<EOF && echo empty-is-char-device
EOF
[ -p /dev/stdin ] <<EOF && echo small-is-pipe
x
EOF
[ -p /dev/stdin ] <<<"$fits" && echo 65536-is-pipe
[ -f /dev/stdin ] <<<"$big" && echo 65537-is-file
[ -r /dev/stdin -a ! -w /dev/stdin ] <<<"$big" && echo file-is-read-only
{ cat >/dev/null; read x; echo "after cat: $? [$x]"; } <<EOF
a
b
EOF
{ read x; cat; } <<EOF
first
second
third
EOF
{ cat; cat; echo "twice"; } <<<"once"
{ wc -c; read -r x; echo "[$x]"; } <<<"$fits"
{ head -c 3; echo; wc -c; } <<<"$big"
{ read -n 2 x; wc -c; } <<<"$big"
{ read -n 5 x; cat; } <<<"$big" | wc -c
wc -c <<<"$big"
cat <<<"$big" | wc -c
cat <<EOF | wc -l
$(printf '%s\n' 1 2 3)
EOF
f() { cat; }; f <<<"in a function"
( cat ) <<<"in a subshell"
{ (cat); read x; echo "subshell consumed: [$x]"; } <<<"gone"
