== stdout 307
in
trapst 1
st 0
trap sees 0
st2 0
usr1
trap sees 0
after2
next
in usr1 0
g 9
1 143
exit2
2 130
exit3
3 143
4 137
5 138
alive
6 0
ignored
7 0
caught
after
8 0
9 138
sent to parent
parent caught
10 0
still ignored
11 0
12 138
13 143 []
14 0 0 0
15 143 0 143
exit 0
16 143
17 141
18 130
int
int
int
term
19 0

== stderr 428
Terminated                 ( kill $BASHPID; echo no )
Terminated                 ( trap 'echo exit3' EXIT; kill -TERM $BASHPID; echo no )
User defined signal 1      ( kill -USR1 $BASHPID; echo "child survived?" )
User defined signal 1      ( trap - USR1; kill -USR1 $BASHPID; echo no )
Terminated                 true | ( kill -TERM $BASHPID )
Terminated                 bash -c 'trap "echo exit \$?" EXIT; kill $$; echo after'

== merged 735
in
trapst 1
st 0
trap sees 0
st2 0
usr1
trap sees 0
after2
next
in usr1 0
g 9
Terminated                 ( kill $BASHPID; echo no )
1 143
exit2
2 130
exit3
Terminated                 ( trap 'echo exit3' EXIT; kill -TERM $BASHPID; echo no )
3 143
4 137
5 138
alive
6 0
ignored
7 0
caught
after
8 0
User defined signal 1      ( kill -USR1 $BASHPID; echo "child survived?" )
9 138
sent to parent
parent caught
10 0
still ignored
11 0
User defined signal 1      ( trap - USR1; kill -USR1 $BASHPID; echo no )
12 138
13 143 []
14 0 0 0
Terminated                 true | ( kill -TERM $BASHPID )
15 143 0 143
exit 0
Terminated                 bash -c 'trap "echo exit \$?" EXIT; kill $$; echo after'
16 143
17 141
18 130
int
int
int
term
19 0

== status 0
