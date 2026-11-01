== stdout 36
one
two
three
four
five
six
seven
1

== stderr 183
>> echo one
>> PS4='[$LINENO] '
[5] echo two
[6] PS4='+$x+ '
++ x=A
+A+ echo three
+A+ PS4=
echo four
unset PS4
echo five
PS4=ab
abecho six
abPS4='%% '
%% echo seven
%% x=1
%% echo 1

== merged 219
>> echo one
one
>> PS4='[$LINENO] '
[5] echo two
two
[6] PS4='+$x+ '
++ x=A
+A+ echo three
three
+A+ PS4=
echo four
four
unset PS4
echo five
five
PS4=ab
abecho six
six
abPS4='%% '
%% echo seven
seven
%% x=1
%% echo 1
1

== status 0
