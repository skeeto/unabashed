== stdout 1097
bash: line 4: printf: x: invalid number
[x] [0.000000]
bash: line 4: printf: 08x: invalid octal number
[08x] [8.000000]
bash: line 4: printf: 0x1g: invalid hex number
[0x1g] [1.000000]
bash: line 4: printf: 1e99999: Numerical result out of range
[1e99999] [inf]
bash: line 4: printf: 1e-99999: Numerical result out of range
[1e-99999] [0.000000]
bash: line 4: printf: 1e-4940: Numerical result out of range
[1e-4940] [0.000000]
bash: line 4: printf: : invalid number
[] [0.000000]
[ 3] [3.000000]
bash: line 4: printf: 3 : invalid number
[3 ] [3.000000]
bash: line 4: printf: 1e: invalid number
[1e] [1.000000]
bash: line 4: printf: 1e+: invalid number
[1e+] [1.000000]
bash: line 4: printf: .: invalid number
[.] [0.000000]
bash: line 4: printf: 0x: invalid hex number
[0x] [0.000000]
bash: line 4: printf: 0x.p1: invalid hex number
[0x.p1] [0.000000]
bash: line 4: printf: nan(: invalid number
[nan(] [nan]
[nan(x_1)] [nan]
[infinity] [inf]
bash: line 4: printf: infinit: invalid number
[infinit] [inf]
[+.5] [0.500000]
[-.5e1] [-5.000000]
[00012.500] [12.500000]
0.000000
status 1
inf
status 1

== stderr 102
bash: line 6: printf: x: invalid number
bash: line 7: printf: 1e999999: Numerical result out of range

== merged 1199
bash: line 4: printf: x: invalid number
[x] [0.000000]
bash: line 4: printf: 08x: invalid octal number
[08x] [8.000000]
bash: line 4: printf: 0x1g: invalid hex number
[0x1g] [1.000000]
bash: line 4: printf: 1e99999: Numerical result out of range
[1e99999] [inf]
bash: line 4: printf: 1e-99999: Numerical result out of range
[1e-99999] [0.000000]
bash: line 4: printf: 1e-4940: Numerical result out of range
[1e-4940] [0.000000]
bash: line 4: printf: : invalid number
[] [0.000000]
[ 3] [3.000000]
bash: line 4: printf: 3 : invalid number
[3 ] [3.000000]
bash: line 4: printf: 1e: invalid number
[1e] [1.000000]
bash: line 4: printf: 1e+: invalid number
[1e+] [1.000000]
bash: line 4: printf: .: invalid number
[.] [0.000000]
bash: line 4: printf: 0x: invalid hex number
[0x] [0.000000]
bash: line 4: printf: 0x.p1: invalid hex number
[0x.p1] [0.000000]
bash: line 4: printf: nan(: invalid number
[nan(] [nan]
[nan(x_1)] [nan]
[infinity] [inf]
bash: line 4: printf: infinit: invalid number
[infinit] [inf]
[+.5] [0.500000]
[-.5e1] [-5.000000]
[00012.500] [12.500000]
bash: line 6: printf: x: invalid number
0.000000
status 1
bash: line 7: printf: 1e999999: Numerical result out of range
inf
status 1

== status 0
