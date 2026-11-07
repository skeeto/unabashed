== stdout 432
bash: line 6: cd: proj: No such file or directory
off 1 /tmp/cv
/tmp/cv/sub
on 0 /tmp/cv/sub
sub
rel 0 /tmp/cv/sub

empty 0 /tmp/cv
bash: line 11: cd: notdir: No such file or directory
notdir 1 /tmp/cv
bash: line 12: cd: unset_var: No such file or directory
unset 1 /tmp/cv
dir wins 0 /tmp/cv/proj
bash: line 14: cd: a b: No such file or directory
badname 1 /tmp/cv
bash: line 15: cd: sbu: No such file or directory
spell 1 /tmp/cv

== stderr 0

== merged 432
bash: line 6: cd: proj: No such file or directory
off 1 /tmp/cv
/tmp/cv/sub
on 0 /tmp/cv/sub
sub
rel 0 /tmp/cv/sub

empty 0 /tmp/cv
bash: line 11: cd: notdir: No such file or directory
notdir 1 /tmp/cv
bash: line 12: cd: unset_var: No such file or directory
unset 1 /tmp/cv
dir wins 0 /tmp/cv/proj
bash: line 14: cd: a b: No such file or directory
badname 1 /tmp/cv
bash: line 15: cd: sbu: No such file or directory
spell 1 /tmp/cv

== status 0
