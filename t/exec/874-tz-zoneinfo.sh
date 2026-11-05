# TZ naming a zoneinfo file: its transitions, the first standard type
# before them, its footer's rule after the last, by absolute path or
# under TZDIR. TZ unset, or not exported, means the system's zone
# (/etc/localtime: UTC in the oracle). Prompts follow the same zone.
hdr='TZif2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'
c1='\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\4'
t1='\0\0\0\0\0\377\377\271\260\0\0EST\0'
t2='\0\0\0\0\0\0\0\0\0\377\377\271\260\0\0EST\0'
printf "$hdr$c1$t1$hdr$c1$t2\nEST5EDT,M3.2.0,M11.1.0\n" >zone
printf "$hdr$c1$t1$hdr$c1$t2\n\n" >nofooter
c1='\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\2\0\0\0\10'
ty='\377\377\271\260\0\0\377\377\307\300\1\4XST\0XDT\0'
t1="\073\232\312\0\101\220\253\0\1\0$ty"
t2="\0\0\0\0\073\232\312\0\0\0\0\0\101\220\253\0\1\0$ty"
printf "$hdr$c1$t1$hdr$c1$t2\nYST3\n" >hist
show() { printf '%s %(%F %T %Z %z)T %(%Z %z)T\n' "${TZ-unset}" 1700000000 1690000000; }
for tz in "$PWD/zone" ":$PWD/nofooter" "$PWD/nosuch" zone; do
    export TZ=$tz
    show
done
for t in 900000000 1000000000 1050000000 1100000000 1200000000; do
    TZ=$PWD/hist printf '%(%F %T %Z %z)T\n' $t
done
export TZDIR=$PWD
TZ=:nofooter
show
TZ=hist
show
unset TZDIR
TZ=:zone
show
unset TZ
show
TZ=EST5EDT
show
export TZ
show
p='\D{%Z %z}'
echo "prompt ${p@P}"
unset TZ
echo "prompt ${p@P}"
export TZ
TZ=$PWD/zone
echo "prompt ${p@P}"
