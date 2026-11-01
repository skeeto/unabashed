# ${x//p/r} with a pattern that matches the empty string: after an empty
# match one whole character is copied, also a multibyte one.
shopt -s extglob
x=aéb
LC_ALL=C.UTF-8
echo "${x//*(z)/-}" "${x//?(é)/-}" "${x//@()/-}" "${x/*(z)/-}"
LC_ALL=C
y=${x//*(z)/-}
echo "${#y}"
LC_ALL=C.UTF-8
v=$'a\xffb'
w=${v//*(z)/-}
printf '%s' "$w" | od -An -c
s=abc
echo "${s//*(c)/.}" "${s//*(x)/.}" "${s//?(a)/.}" "${s//+(c)/.}"
e=
echo "[${e//*(z)/-}]" "[${e//x/-}]"
