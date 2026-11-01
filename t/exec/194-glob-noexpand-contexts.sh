# Contexts where patterns are not expanded against files.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > k1; : > k2
v=k*
show "$v"
show $v
show "${v}"
declare d=k*
show "$d"
local_test() { local l=k*; show "$l"; }
local_test
export ex=k?
show "$ex"
show "${u-k*}"
show ${u-k*}
show ${u-"k*"}
h=k*
read -r r <<EOF2
$h k*
EOF2
show "$r"
