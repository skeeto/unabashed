# conrun: await:a 'type:one\n' await:b 'type:two\n'
# A read that timed out leaves a console read in flight, which feeds
# the next program through a pump; that pump, still reading when its
# program is done, hands the line it reads next to the program after,
# typed once that one runs.
read -t 1 x; echo "st=$?"
: >a; head -n 1
{ sleep 1; : >b; } & head -n 1
wait
echo end
