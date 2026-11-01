# Glob characters in an unquoted ${...} operator word are active;
# quoted ones are not.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > w1; : > w2
show ${u-w*}
show ${u-"w*"}
show ${u-w\*}
show ${u-'w'?}
show "${u-w*}"
v=w
show ${v:+$v?}
show ${u:-w[12]}
set -f
show ${u-w*}
