== stdout 127
no
sub-e trap
st=0
st=1
st=1
st=7
ret 3
sub-ret trap
st=0
st=2
c-unset trap
st=127
c-e trap
st=1
no
c-e2 trap
st=0
no
top trap

== stderr 98
bash: line 6: 1/0: division by 0 (error token is "0")
bash: line 8: u: gone
bash: line 1: u: gone

== merged 225
no
sub-e trap
st=0
bash: line 6: 1/0: division by 0 (error token is "0")
st=1
bash: line 8: u: gone
st=1
st=7
ret 3
sub-ret trap
st=0
st=2
bash: line 1: u: gone
c-unset trap
st=127
c-e trap
st=1
no
c-e2 trap
st=0
no
top trap

== status 0
