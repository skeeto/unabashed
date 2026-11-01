== stdout 109
[via-var]
status 1
status 1
status 1
status 1
status 1
status 0
status 1
status 1
status 1
status 1
status 1

== stderr 465
bash: line 9: $e: ambiguous redirect
bash: line 12: $m: ambiguous redirect
bash: line 14: foo: ambiguous redirect
bash: line 16: bar: ambiguous redirect
bash: line 19: echo: write error: Bad file descriptor
bash: line 24: $c: Bad file descriptor
bash: line 26: 2: Bad file descriptor
bash: line 28: $c: Bad file descriptor
bash: line 30: 3: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 32: 1: Bad file descriptor

== merged 574
[via-var]
bash: line 9: $e: ambiguous redirect
status 1
bash: line 12: $m: ambiguous redirect
status 1
bash: line 14: foo: ambiguous redirect
status 1
bash: line 16: bar: ambiguous redirect
status 1
bash: line 19: echo: write error: Bad file descriptor
status 1
status 0
bash: line 24: $c: Bad file descriptor
status 1
bash: line 26: 2: Bad file descriptor
status 1
bash: line 28: $c: Bad file descriptor
status 1
bash: line 30: 3: Bad file descriptor
status 1
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 32: 1: Bad file descriptor
status 1

== status 0
