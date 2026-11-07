# Messages show odd file names quoted; printf's zone with a width;
# $'\U' beyond the largest code point
exec 2>&1
cd $'no\tsuch'
. $'no\tfile'
hash -p '/a b/c' 'x y'
hash -l
hash -t -l 'x y'
hash -l 'x y'; echo $?
hash -t 'x y'
TZ=UTC0 printf '[%-24.12(%Y-%m-%d %H:%M %Z)T]\n' 86400
TZ=EST5 printf '[%30(%H:%M %Z)T]\n' 0
x=$'\UFFFFFFFF'; echo "${#x}"
x=$'a\U80000000b'; echo "${#x}"
