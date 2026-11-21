# Malformed TZ strings, as glibc's __tzset_parse_tz reads them (no
# zoneinfo file has these names): no name is UTC unnamed, a name
# without an offset UTC by that name, a lone colon "UTC"; anything
# after the standard offset allows daylight saving time, unnamed at
# UTC when no name follows; a rule that does not parse keeps the
# fields glibc read (the rest day 0 at midnight), a missing second
# rule is the United States one, and a rule's time of day is not
# clamped. (A DST name with no rules reads the container's posixrules
# file in glibc, which agrees with the built-in rule only from 2007.)
for tz in 12 :nosuch ABC '<+03>-3' '' : :12 AB 'ABC+' 'ABC+25' \
        'ABC+3:' 'ABC1:2:3:4' 'ABC3 ' 'ABC-3 ' 'ABC0 ' \
        'ABC3DEF,' 'ABC3DEF,M3' 'ABC3DEF,J0,J10' 'ABC3DEF,M4.1.0' \
        'ABC3DEF,M3.2.0,M11.1.0x' 'ABC3DEF,M3.2.0/1:75,M11.1.0/30' \
        'ABC3DEF,M3.2.9,M10.1.0' 'ABC3DEF,M3.0.0,M10.1.0' \
        'ABC3DEF-,M3.2.0,M10.1.0' 'ABC3,M3.2.0,M10.1.0' 'ABC3J100' \
        'ABC3DEF,J400' 'ABC3DEF,M3.2.0/,M10.1.0' 'ABC3DEF,M3.2.0/x,M10.1.0' \
        'ABC3DEF,,M3.2.0' 'ABC3DEF,M3.2.0/-1:30,M10.1.0/+26' \
        'ABC3DEF,M12.5.6/100,M1.1.0' 'ABC3DEF,J65537'; do
    printf '[%s]' "$tz"
    for t in 0 5000000 10000000 15768000 28000000 31500000 1697000000 1700000000; do
        TZ=$tz printf ' %(%Z%z%H)T' $t
    done
    echo
done
