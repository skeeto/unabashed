== stdout 144
pipe right 1
loop 1
pipe right 2
loop 2
loop2 1
loop2 2
read 1
read 3
read [group in pipe 1]
read [group in pipe 2]
group right 1
group right 2

== stderr 306
bash: line 14: break: only meaningful in a `for', `while', or `until' loop
bash: line 14: break: only meaningful in a `for', `while', or `until' loop
bash: line 17: continue: only meaningful in a `for', `while', or `until' loop
bash: line 17: continue: only meaningful in a `for', `while', or `until' loop

== merged 450
pipe right 1
loop 1
pipe right 2
loop 2
loop2 1
loop2 2
read 1
read 3
bash: line 14: break: only meaningful in a `for', `while', or `until' loop
read [group in pipe 1]
bash: line 14: break: only meaningful in a `for', `while', or `until' loop
read [group in pipe 2]
bash: line 17: continue: only meaningful in a `for', `while', or `until' loop
group right 1
bash: line 17: continue: only meaningful in a `for', `while', or `until' loop
group right 2

== status 0
