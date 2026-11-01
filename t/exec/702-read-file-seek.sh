# read takes a regular file in blocks and seeks back to just past its
# line, so whoever reads the file next starts there
printf 'a\nb b\nc\\nd\ne\n' > f
{ read x; read -r y; read z; cat; echo "$x|$y|$z"; } < f
{ read -n 2 x; read y; read -N 3 z; od -c; echo "$x|$y|$z"; } < f
printf 'k1:v1:k2:v2' > g
{ read -d : a; read -d : b; cat; echo; echo "$a $b"; } < g
{ read l; head -c 3; echo; read m; echo "$l|$m"; } < f
exec 3< f
read -u 3 one; read -u 3 two
cat <&3
exec 3<&-
echo "$one|$two"
# lines longer than any block, and a last line without a newline
i=0; s=x; while [ $i -lt 13 ]; do s=$s$s; i=$((i+1)); done
{ echo "$s"; echo short; printf tail; } > h
{ read big; read short; read t; echo "${#big} $short [$t] $?"; } < h
while read -r line; do printf '<%s>' "$line"; done < f; echo
