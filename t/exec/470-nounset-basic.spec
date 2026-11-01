== stdout 85
x=set
def=dflt
def2=dflt2
plus=.
empty=
len=3
st=1
st=1
st=1
st=1
st=1
st=1
st=1
end

== stderr 319
bash: line 9: unset2: unbound variable
bash: line 10: unset3: unbound variable
bash: line 11: unset4: unbound variable
bash: line 12: unset5: unbound variable
bash: line 13: unset6: unbound variable
bash: line 14: unset7: unbound variable
bash: line 15: unset8: unbound variable
bash: line 17: unset9: unbound variable

== merged 404
x=set
def=dflt
def2=dflt2
plus=.
empty=
len=3
bash: line 9: unset2: unbound variable
st=1
bash: line 10: unset3: unbound variable
st=1
bash: line 11: unset4: unbound variable
st=1
bash: line 12: unset5: unbound variable
st=1
bash: line 13: unset6: unbound variable
st=1
bash: line 14: unset7: unbound variable
st=1
bash: line 15: unset8: unbound variable
st=1
end
bash: line 17: unset9: unbound variable

== status 127
