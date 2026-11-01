# ${x#p} and ${x##p}
p() { printf '<%s>' "$@"; echo; }
x=/usr/local/lib/libfoo.so.1
p "${x#*/}" "${x##*/}" "${x#/usr}" "${x##/usr}" "${x#nomatch}"
p "${x#?}" "${x##?}" "${x#*}" "${x##*}"
p "${x#[/u]}" "${x##[!l]*l}" "${x#/[a-z]sr/}"
e=; p "${e#*}" "${e##x}"
unset u; p "${u#a}"
y='a*b*c'
p "${y#*\*}" "${y##*\*}" "${y#a\*}" "${y#'a*'}" "${y#"a*"}"
pat='a*'
p "${y#$pat}" "${y#"$pat"}" "${y##$pat}" "${y##"$pat"}"
