# Quotes and backslashes inside ${...} words within double quotes.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show "${u-'x'}"
show "${u-"x y"}"
show "${u-a\"b}"
show "${u-a\}b}"
show "${u-\$}"
show "${u-\a}"
show ${u-\a}
show ${u-'a\b'}
show "${u-$'\t'}" | { read -r l; printf '%q\n' "$l"; }
v=val
show "${v+'$v'}"
show ${v+'$v'}
show "${v:+"$v"}"
show "${u-${v}}"
show "${u-"${v}"}"
