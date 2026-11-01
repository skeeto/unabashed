== stdout 116
a st=0
b st=0
c st=1
d st=1
f st=1
g st=0 i=1
h st=0
f-body 1
f-body 2
i st=0
g-body 1
g-body 2
sub-body
j st=0
end

== stderr 680
bash: line 1: break: only meaningful in a `for', `while', or `until' loop
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop
bash: line 5: break: 0: loop count out of range
bash: line 7: continue: 0: loop count out of range
bash: line 9: break: -1: loop count out of range
bash: line 15: break: only meaningful in a `for', `while', or `until' loop
bash: line 15: break: only meaningful in a `for', `while', or `until' loop
bash: line 18: continue: only meaningful in a `for', `while', or `until' loop
bash: line 18: continue: only meaningful in a `for', `while', or `until' loop
bash: line 20: break: only meaningful in a `for', `while', or `until' loop

== merged 796
bash: line 1: break: only meaningful in a `for', `while', or `until' loop
a st=0
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop
b st=0
bash: line 5: break: 0: loop count out of range
c st=1
bash: line 7: continue: 0: loop count out of range
d st=1
bash: line 9: break: -1: loop count out of range
f st=1
g st=0 i=1
h st=0
bash: line 15: break: only meaningful in a `for', `while', or `until' loop
f-body 1
bash: line 15: break: only meaningful in a `for', `while', or `until' loop
f-body 2
i st=0
bash: line 18: continue: only meaningful in a `for', `while', or `until' loop
g-body 1
bash: line 18: continue: only meaningful in a `for', `while', or `until' loop
g-body 2
bash: line 20: break: only meaningful in a `for', `while', or `until' loop
sub-body
j st=0
end

== status 0
