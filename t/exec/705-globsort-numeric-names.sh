# GLOBSORT=numeric counts only names of all digits as numbers: a sign,
# a dot, a space or a control byte makes the name a word, and a digit
# string too large for an integer sorts as the largest number.
cd /tmp && mkdir gn && cd gn
touch -- 12 3 -1 +5 1.5 ' 4' '#' '!x' $'\x01' $'\x04B' 5: 0x9 \
      9223372036854775806 1234567890123456789012345 08
show() { printf '%s:' "$1"; shift; printf ' [%q]' "$@"; echo; }
GLOBSORT=numeric;  show numeric *
GLOBSORT=-numeric; show -numeric *
GLOBSORT=numeric;  show prefix 1* +* -*
