# nested expansions inside operator words
p() { printf '<%s>' "$@"; echo; }
unset a b; c=deep; n=3
p "${a:-${b:-${c:-none}}}"
p "${a:-$(echo sub)}" "${a:-`echo bq`}" "${a:-$((n*2))}"
p "${c:+${c}${c}}" "${c:+$((n+1))}"
x=abcabc; y=b
p "${x#*$y}" "${x%$y*}" "${x/$y/${c}}" "${x//${y}/-}"
p "${a:-${#c}}" "${a:-${c#d}}"
p "${a:-"${c:-q}"}"
p "${a:-$(echo "${c%p}")}"
