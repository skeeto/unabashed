# ${s:off:len} counts characters in a UTF-8 locale, bytes in C
s=aéb€cdé
LC_ALL=C.UTF-8
echo "${s:1:2}|${s: -2}|${s:3}|${s:2:-1}|${s:7}|${s:8}|${#s}"
b=$'a\xffé\xe2\x82'
printf '%s' "${b:1:1}" | od -An -tx1
printf '%s' "${b:2:2}" | od -An -tx1
set -- "$s" x
echo "${1:1:3}"
LC_ALL=C
printf '%s' "${s:1:2}" | od -An -tx1
