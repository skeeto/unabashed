== stdout 139
a st=0
b st=1
c st=3
d st=0
e st=1
f st=255
after-g
g st=0
h st=1
after-i
i st=0
j st=4
k st=5
after-l
l st=0
after-m
m st=0
n st=6
o st=9

== stderr 300
bash: line 7: exit: abc: numeric argument required
bash: line 8: exit: too many arguments
bash: line 9: exit: : numeric argument required
bash: line 12: exit: 99999999999999999999: numeric argument required
bash: line 13: exit: 0x10: numeric argument required
bash: line 16: exit: too many arguments

== merged 439
a st=0
b st=1
c st=3
d st=0
e st=1
f st=255
bash: line 7: exit: abc: numeric argument required
after-g
g st=0
bash: line 8: exit: too many arguments
h st=1
bash: line 9: exit: : numeric argument required
after-i
i st=0
j st=4
k st=5
bash: line 12: exit: 99999999999999999999: numeric argument required
after-l
l st=0
bash: line 13: exit: 0x10: numeric argument required
after-m
m st=0
n st=6
o st=9
bash: line 16: exit: too many arguments

== status 1
