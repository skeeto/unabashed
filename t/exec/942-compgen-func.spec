== stdout 291
args: 3 <compgen> <wd> <>
line <> point <0> cword <-1> words 0
r1
r2
st 0
5
one
two
five
st 1
st 1
x
st 0
st 1
x
st 0
args: 3 <compgen> <x> <>
line <> point <0> cword <-1> words 0
pr2
compgen w 
a\$
b$
  l3  $
st 1
x\ny$
z$
st 0 b1 b2
st 1 0
st 1
unalias -- fee
unalias -- fi
unalias -- fum

== stderr 943
bash: line 8: compgen: warning: -F option may not work as you expect
bash: line 10: compgen: warning: -F option may not work as you expect
bash: line 11: compgen: warning: -F option may not work as you expect
bash: line 12: compgen: warning: -F option may not work as you expect
bash: line 14: compgen: warning: -F option may not work as you expect
bash: line 15: compgen: warning: -F option may not work as you expect
bash: line 16: compgen: warning: -F option may not work as you expect
bash: line 17: compgen: warning: -F option may not work as you expect
bash: line 17: completion: function `nofunc' not found
bash: line 19: compgen: warning: -C option may not work as you expect
bash: line 20: compgen: warning: -C option may not work as you expect
bash: line 21: compgen: warning: -C option may not work as you expect
bash: line 22: compgen: warning: -C option may not work as you expect
bash: line 25: compgen: aa: not an indexed array

== merged 1234
bash: line 8: compgen: warning: -F option may not work as you expect
args: 3 <compgen> <wd> <>
line <> point <0> cword <-1> words 0
r1
r2
st 0
5
bash: line 10: compgen: warning: -F option may not work as you expect
one
bash: line 11: compgen: warning: -F option may not work as you expect
two
five
bash: line 12: compgen: warning: -F option may not work as you expect
st 1
bash: line 14: compgen: warning: -F option may not work as you expect
st 1
bash: line 15: compgen: warning: -F option may not work as you expect
x
st 0
bash: line 16: compgen: warning: -F option may not work as you expect
st 1
bash: line 17: compgen: warning: -F option may not work as you expect
bash: line 17: completion: function `nofunc' not found
x
st 0
args: 3 <compgen> <x> <>
line <> point <0> cword <-1> words 0
pr2
bash: line 19: compgen: warning: -C option may not work as you expect
compgen w 
bash: line 20: compgen: warning: -C option may not work as you expect
a\$
b$
  l3  $
bash: line 21: compgen: warning: -C option may not work as you expect
st 1
bash: line 22: compgen: warning: -C option may not work as you expect
x\ny$
z$
st 0 b1 b2
st 1 0
bash: line 25: compgen: aa: not an indexed array
st 1
unalias -- fee
unalias -- fi
unalias -- fum

== status 0
