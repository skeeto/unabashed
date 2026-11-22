== stdout 297
for 1 n=0
select 1 n=0
DEBUG trap - DEBUG
unset %# n=0
null %# n=0
unset /^, n=0
null /^, n=4
substring n=1
errors n=0
no args n=0
no args : n=1
one empty %# n=0
one empty /^ n=2
two empty n=2
empty array n=0
one empty element n=1
empty assoc n=0
indirect n=0
subscript n=1
<><><><>
nounset 1 n=1

== stderr 164
bash: line 5: `0x': not a valid identifier
bash: line 6: `0x': not a valid identifier
bash: line 7: `0y': not a valid identifier
bash: line 34: u: unbound variable

== merged 461
bash: line 5: `0x': not a valid identifier
for 1 n=0
bash: line 6: `0x': not a valid identifier
select 1 n=0
bash: line 7: `0y': not a valid identifier
DEBUG trap - DEBUG
unset %# n=0
null %# n=0
unset /^, n=0
null /^, n=4
substring n=1
errors n=0
no args n=0
no args : n=1
one empty %# n=0
one empty /^ n=2
two empty n=2
empty array n=0
one empty element n=1
empty assoc n=0
indirect n=0
subscript n=1
<><><><>
bash: line 34: u: unbound variable
nounset 1 n=1

== status 0
