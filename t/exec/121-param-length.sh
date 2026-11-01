# ${#name} and special lengths
p() { printf '<%s>' "$@"; echo; }
x=hello; e=; unset u
p "${#x}" "${#e}" "${#u}"
set -- a bb ccc
p "${#}" "${#1}" "${#3}" "${#4}" "${#@}" "${#*}" "${##}"
p "${#0}" "${#?}"
false; p "${#?}"
y='a b  c'; p ${#y}
z=$'\xc3\xa9'; p "${#z}"
set -- 1 2 3 4 5 6 7 8 9 ten eleven; p "${#10}" "${#11}" "${#}"
