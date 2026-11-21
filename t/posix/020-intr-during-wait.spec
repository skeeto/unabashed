== stdout 840
## read x; echo "read $? $x"
status 130 soon
T
read 0 1
status 0 late
## read -n 3 x; echo "read -n $? $x"
status 130 soon
T
read -n 0 1
status 0 late
## read -t 5 x; echo "read -t $? $x"
status 130 soon
T
read -t 0 1
status 0 late
## mapfile -t m; echo "mapfile $? ${m[*]}"
status 130 soon
T
mapfile 0 1
status 0 late
## select v in a b; do echo "v$v"; break; done; echo "select $?"
1) a
2) b
#? status 130 soon
1) a
2) b
#? T
va
select 0
status 0 late
## sleep 1 | cat; echo "pipeline $?"
pipeline 0
status 0 soon
T
pipeline 0
status 0 soon
## (sleep 1); echo "subshell $?"
subshell 0
status 0 soon
T
subshell 0
status 0 soon
## { sleep 1; echo ab; } | { read x; echo "r$x"; }; echo "stage $?"
rab
stage 0
status 0 soon
rab
T
stage 0
status 0 soon
## x=$(sleep 1; echo ab); echo "comsub $? $x"
status 130 soon
T
comsub 0 ab
status 0 soon

== stderr 0

== merged 840
## read x; echo "read $? $x"
status 130 soon
T
read 0 1
status 0 late
## read -n 3 x; echo "read -n $? $x"
status 130 soon
T
read -n 0 1
status 0 late
## read -t 5 x; echo "read -t $? $x"
status 130 soon
T
read -t 0 1
status 0 late
## mapfile -t m; echo "mapfile $? ${m[*]}"
status 130 soon
T
mapfile 0 1
status 0 late
## select v in a b; do echo "v$v"; break; done; echo "select $?"
1) a
2) b
#? status 130 soon
1) a
2) b
#? T
va
select 0
status 0 late
## sleep 1 | cat; echo "pipeline $?"
pipeline 0
status 0 soon
T
pipeline 0
status 0 soon
## (sleep 1); echo "subshell $?"
subshell 0
status 0 soon
T
subshell 0
status 0 soon
## { sleep 1; echo ab; } | { read x; echo "r$x"; }; echo "stage $?"
rab
stage 0
status 0 soon
rab
T
stage 0
status 0 soon
## x=$(sleep 1; echo ab); echo "comsub $? $x"
status 130 soon
T
comsub 0 ab
status 0 soon

== status 0
