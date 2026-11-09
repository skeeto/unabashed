== stdout 665
a 3
b 5
c 5
d 5
e 6
f 40
g <0>
h 9
i 4
j 29
k -9223372036854775808
l -8
m 5 5
n 2 2
o 5
p 9
r 1
s 1
t <0>
u 5
v 5
w 20
x two
y eight
z 11
aa 4
ab 4
ac 5 5
ad <> 2
ae 6
af 1
ag 15
ah 105
bash: line 39: ro: readonly variable
ai status 1
bash: line 40: 1/0: division by 0 (error token is "0")
aj status 1
bash: line 41: 1+: arithmetic syntax error: operand expected (error token is "+")
ak status 1
bash: line 42: u: unbound variable
al status 1
bash: line 43: u: unbound variable
am status 1
ao 3
bash: command substitution: line 45: syntax error near unexpected token `+'
bash: command substitution: line 45: `(1)+((2))'
ap <>
ap status 0
aq 55
ar 6
as 6x
at 6
au 5

== stderr 212
bash: line 21: '1'+1: arithmetic syntax error: operand expected (error token is "'1'+1")
bash: command substitution: line 33: syntax error near unexpected token `+'
bash: command substitution: line 33: `(a)+(b)'

== merged 877
a 3
b 5
c 5
d 5
e 6
f 40
g <0>
h 9
i 4
j 29
k -9223372036854775808
l -8
m 5 5
n 2 2
o 5
p 9
bash: line 21: '1'+1: arithmetic syntax error: operand expected (error token is "'1'+1")
r 1
s 1
t <0>
u 5
v 5
w 20
x two
y eight
z 11
aa 4
ab 4
ac 5 5
bash: command substitution: line 33: syntax error near unexpected token `+'
bash: command substitution: line 33: `(a)+(b)'
ad <> 2
ae 6
af 1
ag 15
ah 105
bash: line 39: ro: readonly variable
ai status 1
bash: line 40: 1/0: division by 0 (error token is "0")
aj status 1
bash: line 41: 1+: arithmetic syntax error: operand expected (error token is "+")
ak status 1
bash: line 42: u: unbound variable
al status 1
bash: line 43: u: unbound variable
am status 1
ao 3
bash: command substitution: line 45: syntax error near unexpected token `+'
bash: command substitution: line 45: `(1)+((2))'
ap <>
ap status 0
aq 55
ar 6
as 6x
at 6
au 5

== status 0
