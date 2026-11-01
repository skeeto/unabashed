== stdout 198
p2 4
p1 3
p1 again 3
%1 7
%1 again 127
all 0
[2]   Done                       sleep 1
n 2
n 1 other
n2 3
none 127
none 127 unset
p 9 same
x 127
y 127
z 1
w 1
o 2
q 2
v 1
r 1
m 127
psub 6
f 0
done 0

== stderr 663
bash: line 12: wait: %1: no such job
bash: line 19: wait: %2: no such job
bash: line 24: wait: pid 12345 is not a child of this shell
bash: line 25: wait: %8: no such job
bash: line 26: wait: `foo': not a pid or valid job spec
bash: line 27: wait: `1x': not a pid or valid job spec
bash: line 28: wait: -x: invalid option
wait: usage: wait [-fn] [-p var] [id ...]
bash: line 29: wait: -p: option requires an argument
wait: usage: wait [-fn] [-p var] [id ...]
bash: line 30: wait: `1bad': not a valid identifier
bash: line 32: wait: RO: cannot unset: readonly variable
bash: line 33: wait: `12345': not a pid or valid job spec
bash: line 33: wait: %9: no such job

== merged 861
p2 4
p1 3
p1 again 3
%1 7
bash: line 12: wait: %1: no such job
%1 again 127
all 0
[2]   Done                       sleep 1
n 2
n 1 other
bash: line 19: wait: %2: no such job
n2 3
none 127
none 127 unset
p 9 same
bash: line 24: wait: pid 12345 is not a child of this shell
x 127
bash: line 25: wait: %8: no such job
y 127
bash: line 26: wait: `foo': not a pid or valid job spec
z 1
bash: line 27: wait: `1x': not a pid or valid job spec
w 1
bash: line 28: wait: -x: invalid option
wait: usage: wait [-fn] [-p var] [id ...]
o 2
bash: line 29: wait: -p: option requires an argument
wait: usage: wait [-fn] [-p var] [id ...]
q 2
bash: line 30: wait: `1bad': not a valid identifier
v 1
bash: line 32: wait: RO: cannot unset: readonly variable
r 1
bash: line 33: wait: `12345': not a pid or valid job spec
bash: line 33: wait: %9: no such job
m 127
psub 6
f 0
done 0

== status 0
