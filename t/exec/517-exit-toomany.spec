== stdout 74
sub st=1
shift st=1
after-break
break st=0
cont st=1
eval st=1
cs st=1 x=

== stderr 313
bash: line 1: exit: too many arguments
bash: line 2: shift: too many arguments
bash: line 3: break: only meaningful in a `for', `while', or `until' loop
bash: line 4: continue: too many arguments
bash: line 5: exit: too many arguments
bash: line 6: exit: too many arguments
bash: line 7: exit: too many arguments

== merged 387
bash: line 1: exit: too many arguments
sub st=1
bash: line 2: shift: too many arguments
shift st=1
bash: line 3: break: only meaningful in a `for', `while', or `until' loop
after-break
break st=0
bash: line 4: continue: too many arguments
cont st=1
bash: line 5: exit: too many arguments
eval st=1
bash: line 6: exit: too many arguments
cs st=1 x=
bash: line 7: exit: too many arguments

== status 1
