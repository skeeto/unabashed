== stdout 105
1 143
2 143
3 0
4 143
5 143
6 0
hi
7 0
8 0
9 143
10 143 0 143
11 143 143 143
12 143
13 143
14 130
15 127

== stderr 463
Terminated                 ( bash -c 'kill $$' )
Terminated                 ( true; bash -c 'kill $$' )
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 true | bash -c 'kill $$'
Terminated                 bash -c 'kill $$' | bash -c 'kill $$'
Terminated                 true | ( true; bash -c 'kill $$' )
Terminated                 bash -c 'kill $$'

== merged 568
1 143
2 143
3 0
Terminated                 ( bash -c 'kill $$' )
4 143
Terminated                 ( true; bash -c 'kill $$' )
5 143
Terminated                 bash -c 'kill $$'
6 0
hi
Terminated                 bash -c 'kill $$'
7 0
8 0
Terminated                 bash -c 'kill $$'
9 143
Terminated                 true | bash -c 'kill $$'
10 143 0 143
Terminated                 bash -c 'kill $$' | bash -c 'kill $$'
11 143 143 143
Terminated                 true | ( true; bash -c 'kill $$' )
12 143
Terminated                 bash -c 'kill $$'
13 143
14 130
15 127

== status 143
