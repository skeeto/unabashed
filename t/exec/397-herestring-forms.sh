# here-strings with descriptors and in compound commands
read -u 3 a 3<<<"on three"; echo "a=$a"
{ read b; read c; } <<<"line1"; echo "status $? b=$b c=[$c]"
f() { read d; echo "d=$d"; }
f <<<"func input"
while read e; do echo "e=$e"; done <<<"x
y"
set -- "p q" r
read g h <<<"$@"; echo "g=$g h=$h"
read g <<<"$*"; echo "g=$g"
s=$(read t <<<"inner"; echo "$t"); echo "s=$s"
read -r v <<<"a\b"; echo "v=$v"
read v <<<"a\b"; echo "v=$v"
