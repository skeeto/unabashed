== stdout 127
got hello
end
before
child read this line
after
z=zzz line
more for cat
y= st=1
again
inner
moved
noeol
st=1
1: OK
3: OK
5: OK

== stderr 83
bash: command substitution: line 22: unexpected EOF while looking for matching `)'

== merged 210
got hello
end
before
child read this line
after
z=zzz line
more for cat
y= st=1
again
inner
moved
noeol
bash: command substitution: line 22: unexpected EOF while looking for matching `)'
st=1
1: OK
3: OK
5: OK

== status 0
