# case modification ^ ^^ , ,,
p() { printf '<%s>' "$@"; echo; }
x='hello World'
p "${x^}" "${x^^}" "${x,}" "${x,,}"
X='HELLO world'
p "${X,}" "${X,,}" "${X^}" "${X^^}"
p "${x^^[lo]}" "${x^[h]}" "${x^[e]}" "${X,,[LO]}" "${x^^[a-m]}"
p "${x^^?}" "${x^*}" "${x^^'o'}"
n='123abc'; p "${n^}" "${n^^}"
e=; unset u; p "${e^^}" "${u,,}"
