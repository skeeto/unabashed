# %(...)T follows an exported TZ: POSIX zone strings and their rules
t=1275250155
for tz in UTC EST5EDT,M3.2.0/2,M11.1.0/2 JST-9 '<+0330>-3:30' foo '' UTC0 \
          'CET-1CEST,M3.5.0,M10.5.0/3' 'NZST-12NZDT,M9.5.0,M4.1.0/3' \
          'AAA3BBB,J60/1,J300' 'AAA3BBB,59,299' 'AAA+3:30:15BBB+2' AB3 ABC \
          ABC3DEF ABC-14 ABC25 'EST5EDT,M3.2.0/-1,M11.1.0/26'; do
    export TZ=$tz
    printf '%s: %(%F %T %Z %z)T | %(%F %T %Z %z)T\n' "$tz" $t 1262304000
done
export TZ='EST5EDT,M3.2.0/2,M11.1.0/2'
for t in 1268550000 1268550060 1268546400 1289109600 1289113200 1289116800; do
    printf '%(%F %T %Z)T\n' $t
done
printf '%(%s %c)T\n' $t
unset TZ; printf 'unset: %(%F %T %Z %z)T\n' $t
TZ=JST-9; printf 'not exported: %(%T %Z)T\n' $t
export -n TZ; TZ=JST-9 printf 'temporary: %(%T %Z)T\n' $t
printf 'after: %(%T %Z)T\n' $t
