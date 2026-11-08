== stdout 528
st=147 0
[1]+  Stopped                    { kill -STOP 0; echo resumed; } | cat
{ kill -STOP 0; echo resumed; } | cat
resumed
st=0 0
st=128
[1]+  Stopped                    true | { kill -TSTP $BASHPID; echo after; }
true | { kill -TSTP $BASHPID; echo after; }
after
st=0
st=147
[1]+  Stopped                    ( kill -STOP $BASHPID; echo in sub; exit 5 )
( kill -STOP $BASHPID; echo in sub; exit 5 )
in sub
st=5
out 128
[1]+  Stopped                    echo $i | f
0 3 1
4 4
x=1
6 6
[1]+  Running                    sleep 1 &

== stderr 0

== merged 528
st=147 0
[1]+  Stopped                    { kill -STOP 0; echo resumed; } | cat
{ kill -STOP 0; echo resumed; } | cat
resumed
st=0 0
st=128
[1]+  Stopped                    true | { kill -TSTP $BASHPID; echo after; }
true | { kill -TSTP $BASHPID; echo after; }
after
st=0
st=147
[1]+  Stopped                    ( kill -STOP $BASHPID; echo in sub; exit 5 )
( kill -STOP $BASHPID; echo in sub; exit 5 )
in sub
st=5
out 128
[1]+  Stopped                    echo $i | f
0 3 1
4 4
x=1
6 6
[1]+  Running                    sleep 1 &

== status 0
