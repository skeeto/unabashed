== stdout 177
sub n=1
comsub n=1 x=comsub n=2 x=comsub n=3 x=
eval n=1
source n=1
bg n=1
pipe n=1
backquote n=1 x=backquote n=2 x=backquote n=3 x=comsub n=2 x=comsub n=3 x=
trap
wow echo wow

== stderr 219
bash: line 8: s1: command not found
bash: line 14: s3: command not found
./f5: line 1: s5: command not found
bash: line 21: s6: command not found
bash: line 24: s7: command not found
bash: line 1: s9: command not found

== merged 396
bash: line 8: s1: command not found
sub n=1
comsub n=1 x=comsub n=2 x=comsub n=3 x=
bash: line 14: s3: command not found
eval n=1
./f5: line 1: s5: command not found
source n=1
bash: line 21: s6: command not found
bg n=1
bash: line 24: s7: command not found
pipe n=1
backquote n=1 x=backquote n=2 x=backquote n=3 x=comsub n=2 x=comsub n=3 x=
bash: line 1: s9: command not found
trap
wow echo wow

== status 0
