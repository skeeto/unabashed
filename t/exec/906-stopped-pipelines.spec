== stdout 440
st=128 0
[1]+  Stopped                    true | bash -c "kill -STOP \$\$"
true | bash -c "kill -STOP \$\$"
st=0 0
st=128
[1]+  Stopped                    echo hi | bash -c "kill -STOP \$\$; cat"
echo hi | bash -c "kill -STOP \$\$; cat"
hi
st=0
st=128
[1]+  Stopped                    f | bash -c "kill -TSTP \$\$; exit 3"
f | bash -c "kill -TSTP \$\$; exit 3"
st=3
[1]+  Stopped                    true | bash -c "kill -TSTP \$\$"
0 1 0
b

== stderr 0

== merged 440
st=128 0
[1]+  Stopped                    true | bash -c "kill -STOP \$\$"
true | bash -c "kill -STOP \$\$"
st=0 0
st=128
[1]+  Stopped                    echo hi | bash -c "kill -STOP \$\$; cat"
echo hi | bash -c "kill -STOP \$\$; cat"
hi
st=0
st=128
[1]+  Stopped                    f | bash -c "kill -TSTP \$\$; exit 3"
f | bash -c "kill -TSTP \$\$; exit 3"
st=3
[1]+  Stopped                    true | bash -c "kill -TSTP \$\$"
0 1 0
b

== status 0
