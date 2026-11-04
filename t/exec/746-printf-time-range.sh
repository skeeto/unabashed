# %(...)T past what localtime can represent prints the epoch instead.
# Both the UTC and the local time must fit a struct tm year, and %Y
# wraps once the year itself passes INT_MAX.
TZ=UTC
for t in 9223372036854775807 -9223372036854775808 99999999999999999999 \
         67768036191676799 67768036191676800 \
         -67768040609740800 -67768040609740801 \
         67767976233532799 67767976233532800; do
    printf '%s %(%Y-%m-%d %T %z %G %C %y)T\n' "$t" "$t"
done
for z in JST-9 EST5EDT,M3.2.0/2,M11.1.0/2 '<-14>14'; do
    export TZ=$z
    for t in 9223372036854775807 -9223372036854775808 \
             67768036191676799 67768036191644399 67768036191644400 \
             -67768040609740800 -67768040609773200 -67768040609708800; do
        printf '%s %s %(%Y-%m-%d %T %z %Z)T\n' "$z" "$t" "$t"
    done
done
