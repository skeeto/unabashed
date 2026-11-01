# quoting in remove patterns inside double quotes
p() { printf '<%s>' "$@"; echo; }
x='abcab'
p "${x#'a'}" "${x%'b'}" "${x#'ab'*}"
x="'a'bc"
p "${x#'a'}" "${x#"'a'"}" "${x#\'}"
y='*star'
p "${y#'*'}" "${y#"*"}" "${y#\*}" "${y#*}"
q='?x'
p "${q#"?"}" "${q#'?'}" "${q#?}"
v='[ab]c'
p "${v#'[ab]'}" "${v#[ab]}" "${v#\[ab\]}"
